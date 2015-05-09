#include <ctype.h>
#include <iostream>
#include <iomanip>
#include <fstream>
#include <string>
#include <cassert>

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/point-to-point-layout-module.h"


#include "ns3/ipv4-global-routing-helper.h"

using namespace ns3;
using namespace std;

// Network topology
//
//       s1 ------------ r1-------------k1
//            8Mbps          0.8 Mbps         
//            0.1 ms          100 ms

// - Flow from s1 to k1 using BulkSendApplication.

static void CwndChange(uint32_t oldCwnd, uint32_t newCwnd) {
    
    std::cout<< Simulator::Now().GetSeconds() << "\t\t" << newCwnd << std::endl;
}

static void TraceCwnd(){
    
        Config::ConnectWithoutContext(
                "/NodeList/0/$ns3::TcpL4Protocol/SocketList/0/CongestionWindow",
                MakeCallback(&CwndChange));
}


int main(int argc, char **argv){

    uint32_t    queueSize   = 64000;
    uint32_t    segSize     = 512;
    float       start_time  = 0.0;
    uint32_t    maxBytes    = 0;

    // Configuration
    Config::SetDefault("ns3::TcpL4Protocol::SocketType", StringValue("ns3::TcpTahoe"));
    Config::SetDefault("ns3::TcpSocket::SegmentSize", UintegerValue (segSize));
    //GlobalValue::Bind ("ChecksumEnabled", BooleanValue (false));
    Config::SetDefault("ns3::TcpSocketBase::WindowScaling", BooleanValue (true));


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
        clientApps.Start (Seconds (start_time));
        clientApps.Stop (Seconds (6.0));

    // TCP Servers    
        
        Address sinkAddress (InetSocketAddress (Ipv4Address::GetAny(),9));
        //Calling the PacketSinkHelper to setup a receiver(sink).
        PacketSinkHelper serverHelper("ns3::TcpSocketFactory", sinkAddress);
        serverApps.Add(serverHelper.Install(s1r1.Get(2)));

        // Setting up simulation times
        serverApps.Start(Seconds (start_time));
        serverApps.Stop(Seconds(6.0));

    Simulator::Schedule(Seconds(start_time + 0.00001), &TraceCwnd);


    std::list<uint32_t> dropList;
    dropList.push_back (10);
    //dropList.push_back (14);
    //dropList.push_back (15);
    //dropList.push_back (16);
    //dropList.push_back (17);


    Ptr<ReceiveListErrorModel> pem = CreateObject<ReceiveListErrorModel> ();
    pem->SetList (dropList);
    routerDevices1.Get(1)->SetAttribute ("ReceiveErrorModel", PointerValue (pem));
    

    Simulator::Stop (Seconds (6.0));
    Simulator::Run();
    

    Simulator::Destroy();

    return 0;
}
