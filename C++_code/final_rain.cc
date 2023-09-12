/*
 *  Simulation experiment to demonstrate my weathered Friis Model.
 *
 *  A simulator to show the change of RSSI power with respect to the distance between
 *  a transimission node and a reciever node over radio.
 *
 *  Feed this file to the file visualise_weather.py in order to generate the graph.
 *
 *  To Run the simulations, write:
 *
 *  "./ns3 run scratch/final_rain"
 *
 *  in the terminal command prompt.
 *
 *  Produces the output file "rssi_time.txt". Feed this into visualise_weather.py to 
 *  generate the desired graph.
 *
 */

// ===================================================================== //
// NS3 Includes
#include "ns3/applications-module.h"
#include "ns3/core-module.h"
#include "ns3/csma-module.h"
#include "ns3/internet-module.h"
#include "ns3/mobility-module.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/ssid.h"
#include "ns3/integer.h"
#include "ns3/yans-wifi-helper.h"
// My includes
#include <fstream>
#include "helpers.hpp"
// note this is dependent on where you have the custome model weatheredfriis header file stored
#include "./weatheredfriis.hpp"

// Default Network Topology
//
//   Wifi 10.1.1.0
//     AP     STA
//     *       *
//     |       |    
//    n0      n2  

using namespace ns3;

std::ofstream ofst{"rssi_time.txt"};

NS_LOG_COMPONENT_DEFINE("TwoNodes");

static void 
SetRainning(Ptr<WeatheredFriisPropagationLossModel> friis, int8_t weatherval){
  std::cout << "In Set Rainning\n";
  friis->SetWeather(weatherval);
}

void 
MonitorSnifferRxCallback( std::string context,
                          Ptr< const Packet > packet, 
                          uint16_t channelFreqMhz, 
                          WifiTxVector txVector, 
                          MpduInfo aMpdu, 
                          SignalNoiseDbm signalNoise,
                          uint16_t staId)
{
  std::cout << context << "\t\t|\t" << Simulator::Now() << "\t|\tPacket of size " << packet->GetSize() 
            << " Recieved with Signal " << signalNoise.signal << " and noise " << signalNoise.noise
            << '\n';
  ofst << Simulator::Now() << ", " << signalNoise.signal << '\n';
}


int
main(int argc, char* argv[])
{
    // ===================================================================== //
 
    // Setup
    bool verbose = false;
    uint32_t nWifi = 2;
    bool tracing = false;

    CommandLine cmd(__FILE__);
    cmd.AddValue("nWifi", "Number of wifi STA devices", nWifi);
    cmd.AddValue("verbose", "Tell echo applications to log if true", verbose);
    cmd.AddValue("tracing", "Enable pcap tracing", tracing);

    cmd.Parse(argc, argv);

    LogComponentEnable("UdpEchoClientApplication", LOG_LEVEL_INFO);
    LogComponentEnable("UdpEchoServerApplication", LOG_LEVEL_INFO);
    LogComponentEnable("UdpClient", LOG_LEVEL_INFO);
    LogComponentEnable("UdpServer", LOG_LEVEL_INFO);

    // ===================================================================== //

    // creation of nodes
    NodeContainer wifiApNode;
    wifiApNode.Create(1);
    NodeContainer wifiStaNode;
    wifiStaNode.Create(1);

    // creation of wifi channel + devices for interconnection between wifi nodes
    // Physical Layer
    YansWifiChannelHelper channel = YansWifiChannelHelper::Default();
    channel.AddPropagationLoss("ns3::WeatheredFriisPropagationLossModel");
    //channel.AddPropagationLoss("ns3::FriisPropagationLossModel");
    YansWifiPhyHelper phy;
    phy.SetChannel(channel.Create());

    // MAC layer
    WifiMacHelper mac;
    Ssid ssid = Ssid("ns-3-ssid");
    // wifi channel 
    WifiHelper wifi;

    // Devices
    NetDeviceContainer staDevice;
    // Read docs - this creates a non ap in a infrastructure BSS
    mac.SetType("ns3::StaWifiMac", "Ssid", SsidValue(ssid), "ActiveProbing", BooleanValue(false));
    // Installing the associated wifi channel + devices + phys + MAC for station nodes
    staDevice = wifi.Install(phy, mac, wifiStaNode);

    NetDeviceContainer apDevice;
    // Settings for accesspoint
    mac.SetType("ns3::ApWifiMac", "Ssid", SsidValue(ssid));
    // installing on wifiApNode, shares the same physical layer.
    apDevice = wifi.Install(phy, mac, wifiApNode);

    // ===================================================================== //

    // Mobility
    // we want Sta nodes to move constantly to the right
    MobilityHelper mobility;

    // mobility for the station nodes  - initial positions for the node
    mobility.SetPositionAllocator("ns3::GridPositionAllocator",
                                  "MinX",
                                  DoubleValue(0.0),
                                  "MinY",
                                  DoubleValue(0.0),
                                  "DeltaX",
                                  DoubleValue(0.5),
                                  "DeltaY",
                                  DoubleValue(0.0),
                                  "GridWidth",
                                  UintegerValue(3),
                                  "LayoutType",
                                  StringValue("RowFirst"));

    // mobility for the ap node - stationary
    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobility.Install(wifiApNode);
    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobility.Install(wifiStaNode);
    /*
    {
        	Ptr<ConstantVelocityMobilityModel> mob = wifiStaNode.Get(0)->GetObject<ConstantVelocityMobilityModel>();
        	mob->SetVelocity(Vector(0.1, 0, 0));
    }
    */
    // ===================================================================== //

    // installing internet stack
    InternetStackHelper stack;
    stack.Install(wifiApNode);
    stack.Install(wifiStaNode);

    // ===================================================================== //
    
    // IP addresses
    Ipv4AddressHelper address;
    address.SetBase("10.1.1.0", "255.255.255.0");
    Ipv4InterfaceContainer staNodeInterface;
    Ipv4InterfaceContainer apNodeInterface;

    staNodeInterface = address.Assign(staDevice);
    apNodeInterface  = address.Assign(apDevice);

    // ===================================================================== //
    
    // Applications
    // server is wifiStaNode
    // client is wifiApNode
    UdpEchoServerHelper echoServer(9);

    ApplicationContainer serverApp = echoServer.Install(wifiApNode.Get(0));
    serverApp.Start(Seconds(1.0));
    serverApp.Stop(Seconds(30));

    UdpEchoClientHelper echoClient(apNodeInterface.GetAddress(0), 9);
    echoClient.SetAttribute("MaxPackets", UintegerValue(100));
    echoClient.SetAttribute("Interval", TimeValue(Seconds(1.0)));
    echoClient.SetAttribute("PacketSize", UintegerValue(1024));

    ApplicationContainer clientApp = echoClient.Install(wifiStaNode.Get(0));
    clientApp.Start(Seconds(2.0));
    clientApp.Stop(Seconds(30.0));

    // ===================================================================== //
    
    if (tracing)
    {
        phy.SetPcapDataLinkType(WifiPhyHelper::DLT_IEEE802_11_RADIO);
        phy.EnablePcap("third", apDevice.Get(0));
    }

    AsciiTraceHelper asciiTraceHelper;
    Ptr<OutputStreamWrapper> stream = asciiTraceHelper.CreateFileStream("mobility.csv");

    // ===================================================================== //
    
    std::ostringstream oss_rssSta;
    oss_rssSta  << "/NodeList/" << wifiStaNode.Get(0)->GetId()
                << "/DeviceList/0/$ns3::WifiNetDevice/Phy/MonitorSnifferRx";
    Config::Connect(oss_rssSta.str(), MakeCallback(&MonitorSnifferRxCallback));

   
    std::ostringstream oss_rssAp;
    oss_rssAp   << "/NodeList/" << wifiApNode.Get(0)->GetId()
                << "/DeviceList/0/$ns3::WifiNetDevice/Phy/MonitorSnifferRx";
    Config::Connect(oss_rssAp.str(), MakeCallback(&MonitorSnifferRxCallback));

    // ===================================================================== //
    Ptr<Channel> ptr = wifiApNode.Get(0)->GetDevice(0)->GetChannel();
    NS_ASSERT(ptr);
    Ptr<YansWifiChannel> dyw = ptr->GetObject<YansWifiChannel>();
    NS_ASSERT(dyw);
    PointerValue ph;
    dyw->GetAttribute("PropagationLossModel", ph);
    Ptr<PropagationLossModel> base_prop = ph.Get<PropagationLossModel>();
    NS_ASSERT(base_prop);
    // downcasting
    Ptr<PropagationLossModel> nextptr = base_prop->GetNext();
    NS_ASSERT(nextptr);

    /*
    Ptr<FriisPropagationLossModel> friis = nextptr->GetObject<FriisPropagationLossModel>();
    NS_ASSERT(friis);
    */

    Ptr<WeatheredFriisPropagationLossModel> friis = nextptr->GetObject<WeatheredFriisPropagationLossModel>();
    NS_ASSERT(friis);
    friis->SetWeather(0);

    // ===================================================================== //

    // Setting the weather conditions

    Simulator::Schedule(Seconds(3), &SetRainning, friis, 0); 
    Simulator::Schedule(Seconds(6), &SetRainning, friis, 1); 
    Simulator::Schedule(Seconds(9), &SetRainning, friis, 2); 
    Simulator::Schedule(Seconds(12), &SetRainning, friis, 1); 
    Simulator::Schedule(Seconds(15), &SetRainning, friis, 2); 
    Simulator::Schedule(Seconds(18), &SetRainning, friis, 1); 
    Simulator::Schedule(Seconds(21), &SetRainning, friis, 0); 

    Simulator::Stop(Seconds(30));
    Simulator::Run();
    Simulator::Destroy();
    return 0;
}
