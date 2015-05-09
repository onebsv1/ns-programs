#include <ctype.h>
#include <iostream>
#include <iomanip>
#include <fstream>
#include <string>
#include <cassert>

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/netanim-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/point-to-point-layout-module.h"


#include "ns3/ipv4-global-routing-helper.h"

using namespace ns3;
using namespace std;

// Network topology
//
//       n0 ------------ n1
//            5 Mbps         
//             10 ms          

// - Flow from n0 to n2 using BulkSendApplication.
static void CwndChange(uint32_t oldCwnd, uint32_t newCwnd) {
    //NS_LOG_UNCOND (Simulator::Now ().GetSeconds () << "\t"/* << newCwnd*/);
    std::cout<< Simulator::Now().GetSeconds() /*<< "\t\t" << oldCwnd */
            << "\t\t" << newCwnd << std::endl;
}

static void TraceCwnd(){
    
        Config::ConnectWithoutContext(
                "/NodeList/0/$ns3::TcpL4Protocol/SocketList/0/CongestionWindow",
                MakeCallback(&CwndChange));
}


int main(int argc, char **argv)
{

    /*RngSeedManager::SetSeed (11223344);
    Ptr<UniformRandomVariable> U = CreateObject<UniformRandomVariable> ();
    U->SetAttribute ("Stream", IntegerValue (6110));
    U->SetAttribute ("Min", DoubleValue (0.0));
    U->SetAttribute ("Max", DoubleValue (0.1));*/

    uint32_t    queueSize = 64000;

    //uint32_t nFlows = 1;
    uint32_t segSize = 512;
    float start_time = 0.0;
    //uint32_t pktSize = 512;
    //string      dRate = "0.1kbps";              //UDP source data rate  

    /*double start[nFlows];

    //Getting all the start times from the random variable.
    for(unsigned int i = 0;i < nFlows; i++){
        start[i] = U->GetValue();
    }*/

    uint32_t maxBytes = 0;

    // Configuration
    Config::SetDefault("ns3::TcpL4Protocol::SocketType", StringValue("ns3::TcpReno"));
    Config::SetDefault("ns3::TcpSocket::SegmentSize", UintegerValue (segSize));
    //GlobalValue::Bind ("ChecksumEnabled", BooleanValue (false));
    Config::SetDefault("ns3::TcpSocketBase::WindowScaling", BooleanValue (true));
    
    /*LogComponentEnable("TcpReno", LOG_LEVEL_INFO);
    LogComponentEnable("PacketSink", LOG_LEVEL_INFO);*/


    NodeContainer s1r1;
    s1r1.Create (3);

    // Setting up network topology
    
    PointToPointHelper Leaf;
    PointToPointHelper Leaf2;

    Leaf.SetDeviceAttribute("DataRate", StringValue("8Mbps"));
    Leaf.SetChannelAttribute("Delay", StringValue("0.1ms"));

    Leaf2.SetDeviceAttribute("DataRate", StringValue("0.8Mbps"));
    Leaf2.SetChannelAttribute("Delay", StringValue("100ms"));

    Leaf.SetQueue("ns3::DropTailQueue","MaxBytes",UintegerValue(queueSize));
    Leaf.SetQueue("ns3::DropTailQueue","Mode",StringValue("QUEUE_MODE_BYTES"));

    Leaf2.SetQueue("ns3::DropTailQueue","MaxBytes",UintegerValue(queueSize));
    Leaf2.SetQueue("ns3::DropTailQueue","Mode",StringValue("QUEUE_MODE_BYTES"));

    NodeContainer net1 (s1r1.Get(0), s1r1.Get(1));
    NodeContainer router1 (s1r1.Get(1), s1r1.Get(2));
    

    NetDeviceContainer nodeDevices1;
    nodeDevices1 = Leaf.Install(net1);

    NetDeviceContainer routerDevices1;
    routerDevices1 = Leaf2.Install (router1);



    InternetStackHelper stack;
    stack.Install(s1r1);

    Ipv4AddressHelper address;
    address.SetBase ("10.1.1.0", "255.255.255.0");
    Ipv4InterfaceContainer p2pInterfaces1;
    p2pInterfaces1 = address.Assign (nodeDevices1);

    Ipv4AddressHelper address2;
    address2.SetBase ("10.1.2.0", "255.255.255.0");
    Ipv4InterfaceContainer p2pInterfaces2;
    p2pInterfaces2 = address2.Assign (routerDevices1);

    AsciiTraceHelper ascii;
    Leaf.EnableAscii (ascii.CreateFileStream ("node0long.tr"),1,0);

    AsciiTraceHelper ascii2;
    Leaf2.EnableAscii (ascii2.CreateFileStream ("node1long.tr"),1,1);


    
    Ipv4GlobalRoutingHelper::PopulateRoutingTables ();
    //In this simulation, the client sends to the server.
    // Setting up applications
    ApplicationContainer clientApps;
    ApplicationContainer serverApps;

    //TCP Clients
    
        //Calling the BulkSendHelper to setup a sender.
        BulkSendHelper clientHelper ("ns3::TcpSocketFactory",InetSocketAddress (p2pInterfaces2.GetAddress(1), 9));
        // Set the amount of data to send in bytes.  Zero is unlimited.
        clientHelper.SetAttribute ("MaxBytes", UintegerValue (maxBytes));
        clientApps = clientHelper.Install (s1r1.Get(0));

        // Setting up simulation times
        clientApps.Start (Seconds (0.0));
        clientApps.Stop (Seconds (6.0));

    // TCP Servers    
        
        Address sinkAddress (InetSocketAddress (Ipv4Address::GetAny(),9));
        //Calling the PacketSinkHelper to setup a receiver(sink).
        PacketSinkHelper serverHelper("ns3::TcpSocketFactory", sinkAddress);
        serverApps.Add(serverHelper.Install(s1r1.Get(2)));

        // Setting up simulation times
        serverApps.Start(Seconds (0.0));
        serverApps.Stop(Seconds(6.0));

    Simulator::Schedule(Seconds(start_time + 0.00001), &TraceCwnd);

    /*//
    // Create one OnOffApplications for UDP and install it on node 0
    //
    DataRate x(dRate);
    OnOffHelper UDPclientHelper ("ns3::TcpSocketFactory", InetSocketAddress (p2pInterfaces2.GetAddress(1), 9));
    UDPclientHelper.SetAttribute("PacketSize", UintegerValue (pktSize));
    UDPclientHelper.SetConstantRate(x,pktSize);
    ApplicationContainer srcApps = clientHelper.Install (s1r1.Get(0));
    srcApps.Start (Seconds (0.0));
    srcApps.Stop (Seconds (6.0));

    //
    // Create a PacketSinkApplication for UDP and install it on node 4
    //
    Address sinkLocalAddress (InetSocketAddress (Ipv4Address::GetAny (), 9));
    PacketSinkHelper packetSinkHelper ("ns3::TcpSocketFactory", sinkLocalAddress);
    ApplicationContainer apps;
    apps.Add (packetSinkHelper.Install (s1r1.Get(2)));
    apps.Start (Seconds (0.0));
    apps.Stop (Seconds (6.0));*/

    std::list<uint32_t> dropList;
    dropList.push_back (16);
    dropList.push_back (29);
    dropList.push_back (30);
    //dropList.push_back (27);

    //dropList.push_back (44);
    //dropList.push_back (85);

    //dropList.push_back (155);



    Ptr<ReceiveListErrorModel> pem = CreateObject<ReceiveListErrorModel> ();
    pem->SetList (dropList);
    routerDevices1.Get(1)->SetAttribute ("ReceiveErrorModel", PointerValue (pem));

    /*Ptr<ReceiveListErrorModel> pem2 = CreateObject<ReceiveListErrorModel> ();
    pem->SetList (dropList);
    nodeDevices1.Get(1)->SetAttribute ("ReceiveErrorModel", PointerValue (pem2));*/
    

    Simulator::Stop (Seconds (6.0));
    Simulator::Run();
    

    Simulator::Destroy();

    return 0;
}
