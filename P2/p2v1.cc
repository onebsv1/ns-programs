#include <iostream>
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/point-to-point-layout-module.h"

// Network topology
//                        n5                    n6
//                        |                     |
//                        |  2 Mbps             |  4 Mbps
//                        |  10 ms              |  10 ms
//                        |                     | 
//       n0 ------------- n1(R)--------------n2(R)---------------n3(R)------------n4
//         
//             5 Mbps           1 Mbps                1 Mbps            5 Mbps 
//             10 ms            20 ms                 20 ms              10 ms
//
// - Flow from n0 to n4 using BulkSendApplication.
// - Flow from n5 to n4 using UdpEchoServer.
// - Flow from n6 to n4 using UdpEchoServer.



using namespace ns3;
using namespace std;

NS_LOG_COMPONENT_DEFINE ("REDExample");



int main(int argc, char **argv)
{

    uint32_t    maxBytes = 0;
    uint32_t    queueSize = 64000;
    uint32_t    windowSize = 64000;
    string      red_dt = "DT";
    double      minTh = 30;
    double      maxTh = 90;
    uint32_t    pktSize = 1024;
    string      bottleNeckLinkBw = "1Mbps";
    string      bottleNeckLinkDelay = "20ms";
    
    CommandLine cmd;
    cmd.AddValue("queueSize","queue size",queueSize);
    cmd.AddValue("windowSize","window size",windowSize);
    cmd.AddValue("red_dt","red_droptail",red_dt);
    cmd.AddValue("minTh","red_droptail",minTh);
    cmd.AddValue("maxTh","red_droptail",maxTh);
    cmd.Parse(argc,argv);

    
    // Options
    GlobalValue::Bind ("ChecksumEnabled", BooleanValue (false));
    Config::SetDefault("ns3::TcpL4Protocol::SocketType", StringValue("ns3::TcpTahoe"));
    Config::SetDefault("ns3::TcpSocketBase::MaxWindowSize", UintegerValue (windowSize));
    Config::SetDefault("ns3::TcpSocketBase::WindowScaling", BooleanValue (false));



    LogComponentEnable("UdpClient", LOG_LEVEL_INFO);
    LogComponentEnable("UdpServer", LOG_LEVEL_INFO);
    LogComponentEnable("TcpSocketBase", LOG_LEVEL_INFO);
    LogComponentEnable("REDExample", LOG_LEVEL_INFO);
    

    NodeContainer p2pNodes;
    p2pNodes.Create (7);



    //
    // Setting up network topology

    PointToPointHelper PTPRouter;
    PointToPointHelper PTPNode01;
    PointToPointHelper PTPNode34;
    PointToPointHelper PTPNode51;
    PointToPointHelper PTPNode62;

    
    if ((red_dt != "RED") && (red_dt != "DT")){
      NS_ABORT_MSG ("Invalid queue type: Use --red_dt=RED or --red_dt=DT");
    }

    if(red_dt == "DT"){
        PTPRouter.SetDeviceAttribute("DataRate", StringValue("1Mbps"));
        PTPRouter.SetChannelAttribute("Delay", StringValue("20ms"));
        PTPRouter.SetQueue("ns3::DropTailQueue","MaxBytes",UintegerValue(queueSize));
        PTPRouter.SetQueue("ns3::DropTailQueue","Mode",StringValue("QUEUE_MODE_BYTES"));
    } else if (red_dt == "RED"){
        minTh *= pktSize; 
        maxTh *= pktSize;
        PTPRouter.SetDeviceAttribute  ("DataRate", StringValue (bottleNeckLinkBw));
        PTPRouter.SetChannelAttribute ("Delay", StringValue (bottleNeckLinkDelay));
        PTPRouter.SetQueue("ns3::RedQueue","QueueLimit",UintegerValue(queueSize));
        PTPRouter.SetQueue("ns3::RedQueue","Mode",StringValue("QUEUE_MODE_BYTES"));
        PTPRouter.SetQueue ("ns3::RedQueue",
                               "MinTh", DoubleValue (minTh),
                               "MaxTh", DoubleValue (maxTh),
                               "LinkBandwidth", StringValue (bottleNeckLinkBw),
                               "LinkDelay", StringValue (bottleNeckLinkDelay));
        
    }

    PTPNode01.SetDeviceAttribute("DataRate", StringValue("5Mbps"));
    PTPNode01.SetChannelAttribute("Delay", StringValue("10ms"));

    PTPNode34.SetDeviceAttribute("DataRate", StringValue("5Mbps"));
    PTPNode34.SetChannelAttribute("Delay", StringValue("10ms"));

    PTPNode51.SetDeviceAttribute("DataRate", StringValue("2Mbps"));
    PTPNode51.SetChannelAttribute("Delay", StringValue("10ms"));

    PTPNode62.SetDeviceAttribute("DataRate", StringValue("4Mbps"));
    PTPNode62.SetChannelAttribute("Delay", StringValue("10ms"));

    //Defines a NodeContainer object that holds Devices
    NodeContainer net1 (p2pNodes.Get(0), p2pNodes.Get(1));
    NodeContainer router1 (p2pNodes.Get(1), p2pNodes.Get(2));
    NodeContainer router2 (p2pNodes.Get(2), p2pNodes.Get(3));
    NodeContainer net4 (p2pNodes.Get(3), p2pNodes.Get(4));
    NodeContainer net51 (p2pNodes.Get(5),p2pNodes.Get(1));
    NodeContainer net62 (p2pNodes.Get(6),p2pNodes.Get(2));

    
    NetDeviceContainer nodeDevices1;
    nodeDevices1 = PTPNode01.Install(net1);

    NetDeviceContainer routerDevices1;
    routerDevices1 = PTPRouter.Install (router1);

    NetDeviceContainer routerDevices2;
    routerDevices2 = PTPRouter.Install (router2);

    NetDeviceContainer nodeDevices2;
    nodeDevices2 = PTPNode34.Install(net4);

    NetDeviceContainer nodeDevices51;
    nodeDevices51 = PTPNode51.Install(net51);

    NetDeviceContainer nodeDevices62;
    nodeDevices62 = PTPNode62.Install(net62);

    
    InternetStackHelper stack;
    stack.Install(p2pNodes);
    

    NS_LOG_INFO ("Assign IP Addresses.");

    Ipv4AddressHelper address;
    address.SetBase ("10.1.1.0", "255.255.255.0");
    Ipv4InterfaceContainer p2pInterfaces1;
    p2pInterfaces1 = address.Assign (nodeDevices1);

    Ipv4AddressHelper address1;
    address1.SetBase ("10.1.2.0", "255.255.255.0");
    Ipv4InterfaceContainer p2pInterfaces2;
    p2pInterfaces2 = address1.Assign (routerDevices1);

    Ipv4AddressHelper address2;
    address2.SetBase ("10.1.3.0", "255.255.255.0");
    Ipv4InterfaceContainer p2pInterfaces3;
    p2pInterfaces3 = address2.Assign (routerDevices2);

    Ipv4AddressHelper address3;
    address3.SetBase ("10.1.4.0", "255.255.255.0");
    Ipv4InterfaceContainer p2pInterfaces4;
    p2pInterfaces4 = address3.Assign (nodeDevices2);

    Ipv4AddressHelper address4;
    address3.SetBase ("10.1.5.0", "255.255.255.0");
    Ipv4InterfaceContainer p2pInterfaces5;
    p2pInterfaces5 = address3.Assign (nodeDevices51);

    Ipv4AddressHelper address5;
    address3.SetBase ("10.1.6.0", "255.255.255.0");
    Ipv4InterfaceContainer p2pInterfaces6;
    p2pInterfaces6 = address3.Assign (nodeDevices62);


    //Populate the routing tables.
    Ipv4GlobalRoutingHelper::PopulateRoutingTables ();   
    
    NS_LOG_INFO ("Create applications.");

    //
    // Create a BulkSendApplication and install it on node 0
    //
     uint16_t port = 43;  // well-known echo port number

    BulkSendHelper source ("ns3::TcpSocketFactory",
                         InetSocketAddress (p2pInterfaces4.GetAddress (1), port));
     // Set the amount of data to send in bytes.  Zero is unlimited.
    source.SetAttribute ("MaxBytes", UintegerValue (maxBytes));
    ApplicationContainer sourceApps = source.Install (p2pNodes.Get (0));
    sourceApps.Start (Seconds (1.0));
    sourceApps.Stop (Seconds (10.0));

    //
    // Create a PacketSinkApplication and install it on node 1
    //
    PacketSinkHelper sink ("ns3::TcpSocketFactory",
                            InetSocketAddress (Ipv4Address::GetAny (), port));
    ApplicationContainer sinkApps = sink.Install (p2pNodes.Get (4));
    sinkApps.Start (Seconds (0.0));
    sinkApps.Stop (Seconds (10.0));


    
    //
    // Setting up UDP sink on n4
    UdpServerHelper server (port);
    ApplicationContainer apps = server.Install (p2pNodes.Get(4));
    apps.Start (Seconds (0.0));
    apps.Stop (Seconds (10.0));

    // UDP Source: n5
    //uint32_t maxPacketCount = 5;
    Time interPacketInterval = Seconds (0.1);
    UdpClientHelper client (p2pInterfaces4.GetAddress (1), port);
    //client.SetAttribute ("MaxPackets", UintegerValue (maxPacketCount));
    client.SetAttribute ("Interval", TimeValue (interPacketInterval));
    client.SetAttribute ("PacketSize", UintegerValue (pktSize));
    apps = client.Install (p2pNodes.Get (5));
    apps.Start (Seconds (0.0));
    apps.Stop (Seconds (10.0));

    // UDP Source: n6
    //uint32_t maxPacketCount2 = 5;
    Time interPacketInterval2 = Seconds (0.1);
    UdpClientHelper client2 (p2pInterfaces4.GetAddress (1), port);
    //client.SetAttribute ("MaxPackets", UintegerValue (maxPacketCount2));
    client2.SetAttribute ("Interval", TimeValue (interPacketInterval2));
    client2.SetAttribute ("PacketSize", UintegerValue (pktSize));
    apps = client.Install (p2pNodes.Get (6));
    apps.Start (Seconds (0.0));
    apps.Stop (Seconds (10.0));
    

    Simulator::Stop (Seconds (10.0));
    Simulator::Run();
    
    Ptr<PacketSink> sink1 = DynamicCast<PacketSink> (sinkApps.Get (0));    
    std::cout << "Goodput: "<< (double)sink1->GetTotalRx () / (double)10.0<< std::endl;

    Simulator::Destroy();

    

    return 0;
}
