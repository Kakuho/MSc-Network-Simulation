/*
 *  Experiment to show the change in rssi versus distance. The set up of the experiment is as follows:
 *
 *
 * Default Network Topology
 *  Wifi 10.1.1.0         _________________
 *    AP     STA         |
 *    *       * -> ->    | TUNNEL
 *    |       |          |
 *    n0      n2         |_________________
 *
 *  There are command line arguments which control the type of walls the tunnel has.
 *  The flag is wallType. 0 is for wood walls, 1 is for concrete walls and 2 is for brick walls.
 *  For example to run an experiment with concrete walls, run the simulation as so:
 *  
 *  ./ns3 run "scratch/final_tunnel --wallType=1"
 *
 *  To generate different graphs for different types of walls, find ofst and change the string to the desired output
 *  name. For example the default output name is rssi_building_brick.txt, on page 49 is the initialisation.
 *
 *  std::ofstream ofst{"rssi_building_brick.txt"};
 *
 *  To generate the graphs for wood or concrete, one can change the name of the file to 
 *  "rssi_building_wood.txt"
 *
 *  and then feed the graphs to visualise_tunnel.py.
 *
 */

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
#include "ns3/buildings-module.h"
// std includes
#include <fstream>
#include <cmath>
// My includes
#include "helpers.hpp"
#include "./kaka/weatheredfriis.hpp"

using namespace ns3;

std::ofstream ofst{"rssi_building_wood.txt"};

NS_LOG_COMPONENT_DEFINE("TwoNodes");

double EuclideanDistance(Vector& v1, Vector& v2){
  double component1 = v1.x - v2.x;
  double component2 = v1.y - v2.y;
  double component3 = v1.z - v2.z;
  return std::sqrt(component1*component1 + component2*component2 + component3*component3);
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
  std::vector<Vector> posvecs;
  for (uint32_t i=0; i < NodeList::GetNNodes(); i++ )
  {
    Ptr<MobilityModel> mob = NodeList::GetNode(i)->GetObject<MobilityModel>();
    Vector pos = mob->GetPosition ();
    posvecs.push_back(pos);
  }
  std::cout << "Distance: " << EuclideanDistance(posvecs[0], posvecs[1]) << '\n';
  // node, distance, signal
  ofst << context[10] << ',' << EuclideanDistance(posvecs[0], posvecs[1]) << ", " << signalNoise.signal << '\n';
}

int
main(int argc, char* argv[])
{
    // Setup
    uint32_t nWifi = 2;
    int wallType = 0;

    CommandLine cmd(__FILE__);
    cmd.AddValue("wallType", "Set the type of the walls. 0 for wood, 1 for concrete and 2 for stone", wallType);
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

    // ===================================================================== //
    
    // Mobility
    // we want Sta nodes to move constantly to the right
    MobilityHelper mobility;

    // initial positions for the nodes
    Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator>();
    positionAlloc->Add(Vector(0.0, 0.0, 1.0));
    positionAlloc->Add(Vector(0.3, 0.0, 1.0));
    mobility.SetPositionAllocator(positionAlloc);

    // mobility for the ap node - stationary
    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobility.Install(wifiApNode);
    mobility.SetMobilityModel("ns3::ConstantVelocityMobilityModel");
    mobility.Install(wifiStaNode);
    // make the node move a bit
    {
          Ptr<ConstantVelocityMobilityModel> mob = wifiStaNode.Get(0)->GetObject<ConstantVelocityMobilityModel>();
          mob->SetVelocity(Vector(0.1, 0, 0));
    }

    // ===================================================================== //

    // creation of buildings
    double x_min = 0.5;
    double x_max = 1.5;
    double y_min = -0.5;
    double y_max = 0.5;
    double z_min = 0.0;
    double z_max = 10.0;
    Ptr<Building> b = CreateObject<Building>();
    b->SetBoundaries(Box(x_min, x_max, y_min, y_max, z_min, z_max));
    b->SetBuildingType(Building::Residential);
    // Wood, ConcreteWithoutWindows, StoneBlocks
    switch(wallType){
      case 0:
        b->SetExtWallsType(Building::Wood);
        break;
      case 1:
        b->SetExtWallsType(Building::ConcreteWithoutWindows);
        break;
      case 2:
        b->SetExtWallsType(Building::StoneBlocks);
        break;
    }

    BuildingsHelper::Install(wifiApNode);
    BuildingsHelper::Install(wifiStaNode);

    // ===================================================================== //
    
    // creation of wifi channel + devices for interconnection between wifi nodes
    
    // Physical Layer
    YansWifiChannelHelper channel = YansWifiChannelHelper::Default();
    //channel.AddPropagationLoss("ns3::WeatheredFriisPropagationLossModel");
    //channel.AddPropagationLoss("ns3::FriisPropagationLossModel");
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
    UdpEchoServerHelper echoServer(9);

    ApplicationContainer serverApp = echoServer.Install(wifiApNode.Get(0));
    serverApp.Start(Seconds(1.0));
    serverApp.Stop(Seconds(10.0));

    UdpEchoClientHelper echoClient(apNodeInterface.GetAddress(0), 9);
    echoClient.SetAttribute("MaxPackets", UintegerValue(100));
    echoClient.SetAttribute("Interval", TimeValue(Seconds(1.0)));
    echoClient.SetAttribute("PacketSize", UintegerValue(1024));

    ApplicationContainer clientApp = echoClient.Install(wifiStaNode.Get(0));
    clientApp.Start(Seconds(2.0));
    clientApp.Stop(Seconds(10.0));

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

    Simulator::Stop(Seconds(20));
    Simulator::Run();
    Simulator::Destroy();
    return 0;
}
