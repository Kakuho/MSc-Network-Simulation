/*  Code for the Mobility Experiment
 *  
 *  The network is using YansChannelWifi and we use schedule a call to PrintPositions every second in order to 
 *  log positional data out.
 *
 *  To run, invoke ns3 by running 
 *
 *  "./ns3 run scratch/mobiFinal"
 *  
 *  You can also change the number of mobile nodes in the simulation by using the commands
 *
 *  "./ns3 run "scratch/mobiFinal --nWifi=5"
 *
 *  This creates the experiment with 5 nodes in the simulation.
 *
 *  Another command line option is the mobility option. Set 0 for randomwalk and 1 for randomwaypoint
 *
 *  "./ns3 run "scratch/mobiFinal --nWifi=8 --mobiOption=1"
 *
 *  Creates a simulation with 8 nodes using the randomwaypoint model.
 *
 *  Another option is timeOption. Set 0 for 50 seconds and 1 for 1 hour. You invoke this setting as the previous cli
 *  flags.
 *
 *  The generated file is called mobility.txt. Feed this into visualise_mobility.py to generate 
 *  the desired graph
 */

#include "fstream"
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
#include "ns3/olsr-helper.h"
// My includes
#include "helpers.hpp"
#include "./kaka/weatheredfriis.hpp"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("Adhoc");

std::ofstream ofst{"mobility.txt", std::ios::out | std::ios::trunc};

// The function which allows us to get output of positions. Prints to std::cout and to ofst
void PrintPositions ()
{
  for (uint32_t i=0; i < NodeList::GetNNodes(); i++ )
  {
    Ptr<MobilityModel> mob = NodeList::GetNode(i)->GetObject<MobilityModel>();
    Vector pos = mob->GetPosition ();
    std::cout << "Node " << i << " | POS: x=" << pos.x << ", y=" << pos.y << '\n';
    ofst << i << ", " << pos.x << ", " << pos.y << "\n";
  }
  Simulator::Schedule(Seconds(1), MakeCallback(&PrintPositions));
}

int
main(int argc, char* argv[])
{
    // ===================================================================== //
 
    // Setup
    uint32_t nWifi = 10;
    int mobiOption = 0;
    int timeOption = 0;

    CommandLine cmd(__FILE__);
    cmd.AddValue("nWifi", "Number of moving nodes", nWifi);
    cmd.AddValue("mobiOption", "set 0 for randomwalk, 1 for randomwaypoint", mobiOption);
    cmd.AddValue("timeOption", "set 0 for 50 seconds, 1 for 1 hour", timeOption);
    cmd.Parse(argc, argv);

    // ===================================================================== //

    // creation of nodes
    NodeContainer adhocNodes;
    adhocNodes.Create(nWifi);

    // creation of wifi channel + devices for interconnection between wifi nodes
    // Physical Layer
    YansWifiPhyHelper phy;
    // channels
    YansWifiChannelHelper channel = YansWifiChannelHelper::Default();
    channel.AddPropagationLoss("ns3::FriisPropagationLossModel"); /** MONKE **/
    phy.SetChannel(channel.Create());
    // MAC layer
    WifiMacHelper mac;
    Ssid ssid = Ssid("ns-3-ssid");
    // wifi channel 
    WifiHelper wifi;
    wifi.SetStandard(WIFI_STANDARD_80211b);
    // Set to adhoc mode
    mac.SetType("ns3::AdhocWifiMac");
    // Devices
    NetDeviceContainer devices = wifi.Install(phy, mac, adhocNodes);

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
                                  DoubleValue(1.0),
                                  "DeltaY",
                                  DoubleValue(1.0),
                                  "GridWidth",
                                  UintegerValue(6),
                                  "LayoutType",
                                  StringValue("RowFirst"));

    switch(mobiOption){
      case 0:
        // randomwalk with bounding box
        mobility.SetMobilityModel("ns3::RandomWalk2dMobilityModel", 
                                  "Bounds",
                                  RectangleValue(Rectangle(-50, 50, -50, 50))
                                  );
        break;
      case 1:
        // Random WayPoint 
        ObjectFactory pos;
        pos.SetTypeId("ns3::RandomRectanglePositionAllocator");
        pos.Set("X", StringValue("ns3::UniformRandomVariable[Min=-50.0|Max=50.0]"));
        pos.Set("Y", StringValue("ns3::UniformRandomVariable[Min=-50.0|Max=50.0]"));
        Ptr<PositionAllocator> taPositionAlloc = pos.Create()->GetObject<PositionAllocator>();
        mobility.SetMobilityModel("ns3::RandomWaypointMobilityModel", 
                                  "Speed",
                                  StringValue("ns3::UniformRandomVariable[Min=1.0|Max=5.0]"),
                                  "Pause",
                                  StringValue("ns3::UniformRandomVariable[Min=1.0|Max=5.0]"),
                                  "PositionAllocator",
                                  PointerValue(taPositionAlloc)
                                  );
        break;
    }
    mobility.Install(adhocNodes);

    // ===================================================================== //

    // Enabling MANET routing - OLSR
    OlsrHelper olsr;
    Ipv4StaticRoutingHelper staticRouting;
    // configure routings
    Ipv4ListRoutingHelper ipList;
    ipList.Add(staticRouting, 0);
    // make olsr have higher priority over static routing
    ipList.Add(olsr, 10);

    // installing internet stack
    InternetStackHelper internet;
    internet.SetRoutingHelper(ipList);
    internet.Install(adhocNodes);

    // ===================================================================== //
    
    // IP addresses
    Ipv4AddressHelper address;
    address.SetBase("10.1.1.0", "255.255.255.0");
    Ipv4InterfaceContainer interfaces = address.Assign(devices);

    // ===================================================================== //

    // you need to put Simulator::Schedule in front of Simulator::Run();
    //Simulator::Schedule(Seconds(2), &Print, "oioi");
    Simulator::Schedule(Seconds(0), &PrintPositions); 
    Simulator::Stop(Hours(1));
    Simulator::Run();
    Simulator::Destroy();
    return 0;
}
