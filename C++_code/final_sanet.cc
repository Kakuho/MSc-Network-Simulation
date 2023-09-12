// Program adapted from manet_compare.cc
//
// This simulation creates a SANET simulation, where there are small quick ships and medium slower ships.
// To run, write the following in the command prompt:
//
// "./ns3 run scratch/final_sanet"
//
// This will produce the file sanet.output.csv. Feed this file into the visualise_sanet.py in order to generate the
// graph.

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

#include "./kaka/weatheredfriis.hpp"

#include <fstream>
#include <iostream>
#include <vector>

using namespace ns3;
using namespace dsr;

NS_LOG_COMPONENT_DEFINE("manet-routing-compare");

static void
SetRainning(NodeContainer& nc, uint32_t size, int8_t weatherval){
  for(uint32_t i = 0; i < size; i++){
    Ptr<Channel> ptr = nc.Get(i)->GetDevice(0)->GetChannel();
    NS_ASSERT(ptr);
    Ptr<YansWifiChannel> dyw = ptr->GetObject<YansWifiChannel>();
    NS_ASSERT(dyw);
    PointerValue ph;
    dyw->GetAttribute("PropagationLossModel", ph);
    Ptr<PropagationLossModel> base_prop = ph.Get<PropagationLossModel>();
    NS_ASSERT(base_prop);
    Ptr<WeatheredFriisPropagationLossModel> friis = base_prop->GetObject<WeatheredFriisPropagationLossModel>();
    NS_ASSERT(friis);
    friis->SetWeather(weatherval);
  }
}

static uint32_t packetsSent{0};

class RoutingExperiment
{
  public:
    RoutingExperiment();
    void Run();

    void CommandSetup(int argc, char** argv);

  private:
    Ptr<Socket> SetupPacketReceive(Ipv4Address addr, Ptr<Node> node);
    void ReceivePacket(Ptr<Socket> socket);
    void CheckThroughput();

    uint32_t port{9};            //!< Receiving port number.
    uint32_t bytesTotal{0};      //!< Total received bytes.
    int packetsReceived{0};      //!< Total received packets.

    std::string m_CSVfileName{"sanet.output.csv"};         //!< CSV filename.
    int m_nSinks{10};                                      //!< Number of sink nodes.
    std::string m_protocolName{"AODV"};                    //!< Protocol name.
    double m_txp{7.5};                                     //!< Tx power.
    bool m_traceMobility{false};                           //!< Enable mobility tracing.
    bool m_flowMonitor{false};                             //!< Enable FlowMonitor.
    std::vector<double> delays;
};

RoutingExperiment::RoutingExperiment()
{
}

double starttime = 0; // start of package sent

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
    CommandLine cmd(__FILE__);
    cmd.Parse(argc, argv);
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

    // blank out the last output file and write the column headers
    std::ofstream out{m_CSVfileName, std::ios::trunc};
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

    int nSmallNodes = 45;
    int nMediumNodes = 25;
    int nWifis = nSmallNodes + nMediumNodes;

    double TotalTime = 250.0;
    std::string rate("2048bps");
    std::string phyMode("DsssRate11Mbps");
    std::string tr_name("manet-routing-compare");
    int nodeSpeed = 20; // in m/s
    int nodePause = 0;  // in s

    Config::SetDefault("ns3::OnOffApplication::PacketSize", StringValue("64"));
    Config::SetDefault("ns3::OnOffApplication::DataRate", StringValue(rate));

    // Set Non-unicastMode rate to unicast mode
    Config::SetDefault("ns3::WifiRemoteStationManager::NonUnicastMode", StringValue(phyMode));

    // -------------------------------------------------------------------------------------- //
    
    // Topology
    NodeContainer adhocNodes;
    adhocNodes.Create(nWifis);

    NodeContainer smallShips;
    NodeContainer mediumShips;

    for(uint32_t n = 0; n < nWifis ; n++){
        Ptr<Node> node = adhocNodes.Get(n);
      if(n < 45){
        smallShips.Add(node);
      }
      else{
        mediumShips.Add(node);
      }
    }

    // -------------------------------------------------------------------------------------- //
    
    // Devices
    // setting up wifi phy and channel using helpers
    WifiHelper wifi;
    wifi.SetStandard(WIFI_STANDARD_80211b);

    YansWifiPhyHelper wifiPhy;
    YansWifiChannelHelper wifiChannel;
    wifiChannel.SetPropagationDelay("ns3::ConstantSpeedPropagationDelayModel");
    wifiChannel.AddPropagationLoss("ns3::WeatheredFriisPropagationLossModel");
    wifiPhy.SetChannel(wifiChannel.Create());

    // Add a mac and disable rate control
    WifiMacHelper wifiMac;
    wifi.SetRemoteStationManager("ns3::ConstantRateWifiManager",
                                 "DataMode",
                                 StringValue(phyMode),
                                 "ControlMode",
                                 StringValue(phyMode));

    wifiPhy.Set("TxPowerStart", DoubleValue(m_txp));
    wifiPhy.Set("TxPowerEnd", DoubleValue(m_txp));

    wifiMac.SetType("ns3::AdhocWifiMac");
    NetDeviceContainer adhocDevices = wifi.Install(wifiPhy, wifiMac, adhocNodes);

    // -------------------------------------------------------------------------------------- //
    // Mobility
    
    MobilityHelper mobilitySmallShips;
    MobilityHelper mobilityMediumShips;

    // Position allocator

    ObjectFactory pos;
    pos.SetTypeId("ns3::RandomDiscPositionAllocator");
    pos.Set("Rho", StringValue("ns3::UniformRandomVariable[Min=0.0|Max=300.0]")); 
    // radius of the random disc
    pos.Set("Theta", StringValue("ns3::UniformRandomVariable[Min=0.0|Max=360.0]"));  
    // angle of position

    Ptr<PositionAllocator> taPositionAlloc = pos.Create()->GetObject<PositionAllocator>();
    mobilitySmallShips.SetPositionAllocator(taPositionAlloc);
    mobilityMediumShips.SetPositionAllocator(taPositionAlloc);

    // Position allocator for rwp

    pos.SetTypeId("ns3::RandomRectanglePositionAllocator");
    pos.Set("X", StringValue("ns3::UniformRandomVariable[Min=-100.0|Max=100.0]")); 
    // radius of the random disc
    pos.Set("Y", StringValue("ns3::UniformRandomVariable[Min=-100.0|Max=100.0]"));  
    // angle of position
    Ptr<PositionAllocator> rwpPositionAlloc = pos.Create()->GetObject<PositionAllocator>();

    // Set mobility model 

    mobilitySmallShips.SetMobilityModel(  
                                        "ns3::RandomWaypointMobilityModel",
                                        "Speed",
                                        StringValue("ns3::UniformRandomVariable[Min=100|Max=200]"),
                                        "Pause",
                                        StringValue("ns3::ConstantRandomVariable[Constant=2.0]"),
                                        "PositionAllocator",
                                        PointerValue(rwpPositionAlloc)
                                        );

    mobilityMediumShips.SetMobilityModel(  
                                        "ns3::RandomWaypointMobilityModel",
                                        "Speed",
                                        StringValue("ns3::UniformRandomVariable[Min=10|Max=50]"),
                                        "Pause",
                                        StringValue("ns3::ConstantRandomVariable[Constant=2.0]"),
                                        "PositionAllocator",
                                        PointerValue(rwpPositionAlloc)
                                        );

    mobilitySmallShips.Install(smallShips);
    mobilityMediumShips.Install(mediumShips);
    
    // -------------------------------------------------------------------------------------- //

    // Routing in adhoc + internet stack + ipv4
    AodvHelper aodv;
    Ipv4ListRoutingHelper list;
    InternetStackHelper internet;

    list.Add(aodv, 100);
    internet.SetRoutingHelper(list);
    internet.Install(adhocNodes);
    NS_LOG_INFO("assigning ip address");

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

    Simulator::Schedule(Seconds(200), &SetRainning, smallShips, nSmallNodes, 1);
    Simulator::Stop(Seconds(TotalTime));
    Simulator::Run();

    Simulator::Destroy();
}
