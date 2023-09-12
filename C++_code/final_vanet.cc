// NS3  and SUMO in conjuction.
//
// Program adapted from final_sanet.cc
//
// This simulation creates a VANET simulation, where the vehicles move according to a trace generated from SUMO.
// To run, write the following in the command prompt:
//
// "./ns3 run scratch/final_vanet"
//

#include "ns3/aodv-module.h"
#include "ns3/applications-module.h"
#include "ns3/core-module.h"
#include "ns3/dsdv-module.h"
#include "ns3/dsr-module.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/internet-module.h"
#include "ns3/mobility-module.h"
#include "ns3/network-module.h"
#include "ns3/olsr-module.h"
#include "ns3/yans-wifi-helper.h"
#include "ns3/mobility-module.h"
#include "ns3/ns2-mobility-helper.h"
#include "ns3/buildings-module.h"

#include "./kaka/weatheredfriis.hpp"

#include <fstream>
#include <iostream>
#include <vector>

using namespace ns3;
using namespace dsr;

NS_LOG_COMPONENT_DEFINE("manet-routing-compare");

static uint32_t packetsSent{0};

class RoutingExperiment
{
  public:
    RoutingExperiment() = default;
    void Run();
    void CommandSetup(int argc, char** argv);

  private:
    Ptr<Socket> SetupPacketReceive(Ipv4Address addr, Ptr<Node> node);
    void ReceivePacket(Ptr<Socket> socket);
    void CheckThroughput();

    uint32_t port{9};             //!< Receiving port number.
    uint32_t bytesTotal{0};       //!< Total received bytes.
    int packetsReceived{0};       //!< Total received packets.

    std::string m_CSVfileName{"vanet.csv"};                //!< CSV filename.
    int m_nSinks{10};                                      //!< Number of sink nodes.
    std::string m_protocolName{"AODV"};                    //!< Protocol name.
    double m_txp{7.5};                                     //!< Tx power.
    bool m_traceMobility{false};                           //!< Enable mobility tracing.
    bool m_flowMonitor{false};                             //!< Enable FlowMonitor.
    std::vector<double> delays;
};

double starttime = 0;

static inline std::string
PrintReceivedPacket(Ptr<Socket> socket, Ptr<Packet> packet, Address senderAddress)
{
    std::ostringstream oss;

    oss << Simulator::Now().GetSeconds() << " " << socket->GetNode()->GetId();

    if (InetSocketAddress::IsMatchingType(senderAddress))
    {
        InetSocketAddress addr = InetSocketAddress::ConvertFrom(senderAddress);
        oss << " received one packet from " << addr.GetIpv4();
    }
    else
    {
        oss << " received one packet!";
    }
    return oss.str();
}

void
RoutingExperiment::ReceivePacket(Ptr<Socket> socket)
{
    Ptr<Packet> packet;
    Address senderAddress;
    while ((packet = socket->RecvFrom(senderAddress)))
    {
      SeqTsHeader hdr;
      double received = hdr.GetTs().GetSeconds();
      packet->PeekHeader(hdr);
      double delta = received - starttime;
      delays.push_back(delta);

      // ===================================================================== //

      packetsReceived +=1;

      // ===================================================================== //

        bytesTotal += packet->GetSize();
        NS_LOG_UNCOND(PrintReceivedPacket(socket, packet, senderAddress));
    }
}

void
RoutingExperiment::CheckThroughput()
{
    double kbs = (bytesTotal * 8.0) / 1000;
    bytesTotal = 0;

    // ===================================================================== //
    
    float pdr = static_cast<float>(packetsReceived)/static_cast<float>(packetsSent);

    // ===================================================================== //

    double totale2e = 0;
    for(double k: delays){
      totale2e += k;
    }
    double average_e2e = totale2e/packetsReceived;
    delays.erase(delays.begin(), delays.end());

    // ===================================================================== //
    
    std::ofstream out(m_CSVfileName, std::ios::app);

    out << (Simulator::Now()).GetSeconds() << "," << kbs << "," << packetsReceived << ","
        << average_e2e << "," << pdr << ","
        << m_nSinks << "," << m_protocolName << "," << m_txp << "" << std::endl;

    out.close();
    packetsReceived = 0;
    packetsSent = 0;
    Simulator::Schedule(Seconds(1.0), &RoutingExperiment::CheckThroughput, this);
}

Ptr<Socket>
RoutingExperiment::SetupPacketReceive(Ipv4Address addr, Ptr<Node> node)
{
    TypeId tid = TypeId::LookupByName("ns3::UdpSocketFactory");
    Ptr<Socket> sink = Socket::CreateSocket(node, tid);
    InetSocketAddress local = InetSocketAddress(addr, port);
    sink->Bind(local);
    sink->SetRecvCallback(MakeCallback(&RoutingExperiment::ReceivePacket, this));
    return sink;
}

static void
Tx( std::string c, 
    Ptr<const Packet> packet
  ){
  packetsSent+=1;
  SeqTsHeader hdr;
  starttime = hdr.GetTs().GetSeconds();
}

void
RoutingExperiment::CommandSetup(int argc, char** argv)
{
}

int
main(int argc, char* argv[])
{
    RoutingExperiment experiment;
    experiment.CommandSetup(argc, argv);
    experiment.Run();
    return 0;
}

void
RoutingExperiment::Run()
{
    Packet::EnablePrinting();
    std::ofstream out{m_CSVfileName + "output.csv"};
    out << "SimulationSecond,"
        << "ReceiveRate,"
        << "PacketsReceived,"
        << "Average End to End,"
        << "Package Delivery Ratio,"
        << "NumberOfSinks,"
        << "RoutingProtocol,"
        << "TransmissionPower" << std::endl;
    out.close();

    // Setup
    std::uint8_t nVehicles = 450; // there will be an integer overflow as 450 > 2^8
    double TotalTime = 3615.0;
    std::string rate("2048bps");
    std::string phyMode("DsssRate11Mbps");
    std::string tr_name("manet-routing-compare");

    Config::SetDefault("ns3::OnOffApplication::PacketSize", StringValue("64"));
    Config::SetDefault("ns3::OnOffApplication::DataRate", StringValue(rate));

    // Set Non-unicastMode rate to unicast mode
    Config::SetDefault("ns3::WifiRemoteStationManager::NonUnicastMode", StringValue(phyMode));

    // -------------------------------------------------------------------------------------- //
    
    // Network Topology
    NodeContainer vehicles;
    vehicles.Create(nVehicles);

    NodeContainer adhocNodes;
    adhocNodes = vehicles;         // shallow copy

    // -------------------------------------------------------------------------------------- //

     // creation of wifi channel + devices for interconnection between wifi nodes

    // Physical Layer
    YansWifiChannelHelper channel = YansWifiChannelHelper::Default();
    channel.SetPropagationDelay("ns3::ConstantSpeedPropagationDelayModel");
    channel.AddPropagationLoss("ns3::HybridBuildingsPropagationLossModel",
                                    "CitySize", StringValue("Small"),
                                    "ShadowSigmaOutdoor", DoubleValue (10.0),
                                    "ShadowSigmaExtWalls", DoubleValue (10.0),
                                    "InternalWallLoss", DoubleValue (10.0),
                                    "Environment", StringValue("Urban")
                              );
    YansWifiPhyHelper phy;
    phy.SetChannel(channel.Create());
    // MAC layer
    WifiMacHelper wifiMac;
    // wifi channel
    WifiHelper wifi;
    wifi.SetStandard(WIFI_STANDARD_80211b);
    wifi.SetRemoteStationManager("ns3::ConstantRateWifiManager",
                                 "DataMode",
                                 StringValue(phyMode),
                                 "ControlMode",
                                 StringValue(phyMode));
    wifiMac.SetType("ns3::AdhocWifiMac");
    // Devices
    NetDeviceContainer adhocDevices = wifi.Install(phy, wifiMac, vehicles);

    // -------------------------------------------------------------------------------------- //

    // mobility
    std::string mobility_file_name{"./scratch/cardiff.tcl"};          // note this is a relative path from where ns3 is
                                                                      // stored
    Ns2MobilityHelper ns2 = Ns2MobilityHelper(mobility_file_name);
    ns2.Install();
    
    // -------------------------------------------------------------------------------------- //

    // Buildings
    double x_min = 0.5;
    double x_max = 1.5;
    double y_min = -0.5;
    double y_max = 0.5;
    double z_min = 0.0;
    double z_max = 10.0;
    Ptr<Building> b = CreateObject<Building>();
    b->SetBoundaries(Box(x_min, x_max, y_min, y_max, z_min, z_max));
    b->SetBuildingType(Building::Residential);
    b->SetExtWallsType(Building::StoneBlocks);
    BuildingsHelper::Install(vehicles);

    // ===================================================================== //
    
    // Routing in adhoc + internet stack + ipv4
    AodvHelper aodv;
    Ipv4ListRoutingHelper list;
    InternetStackHelper internet;

    list.Add(aodv, 100);
    internet.SetRoutingHelper(list);
    internet.Install(adhocNodes);

    // -------------------------------------------------------------------------------------- //
    
    // ip address + masking
    Ipv4AddressHelper addressAdhoc;
    addressAdhoc.SetBase("10.1.1.0", "255.255.255.0");
    Ipv4InterfaceContainer adhocInterfaces;
    adhocInterfaces = addressAdhoc.Assign(adhocDevices);

    OnOffHelper onoff1("ns3::UdpSocketFactory", Address());
    onoff1.SetAttribute("OnTime", StringValue("ns3::ConstantRandomVariable[Constant=1.0]"));
    onoff1.SetAttribute("OffTime", StringValue("ns3::ConstantRandomVariable[Constant=0.0]"));

    for (int i = 0; i < m_nSinks; i++)
    {
      // setting up source and sinks
        Ptr<Socket> sink = SetupPacketReceive(adhocInterfaces.GetAddress(i), adhocNodes.Get(i));

        AddressValue remoteAddress(InetSocketAddress(adhocInterfaces.GetAddress(i), port));
        onoff1.SetAttribute("Remote", remoteAddress); // remote address is the destination

        Ptr<UniformRandomVariable> var = CreateObject<UniformRandomVariable>(); // random number
        ApplicationContainer temp = onoff1.Install(adhocNodes.Get(i + m_nSinks)); // install a onoff sender at i +
                                                                                  // m_nSinks, who send it to node i
        std::ostringstream oss;
        oss << "/NodeList/" << adhocNodes.Get(i + m_nSinks)->GetId()
            << "/ApplicationList/0/$ns3::OnOffApplication/Tx";
        Config::Connect(oss.str(), MakeCallback(&Tx));

        temp.Start(Seconds(var->GetValue(100.0, 101.0)));
        temp.Stop(Seconds(TotalTime));
        
    }

    // ===================================================================== //
    
    NS_LOG_INFO("Run Simulation.");
    CheckThroughput();
    Simulator::Stop(Seconds(1000));
    Simulator::Run();
    Simulator::Destroy();
}
