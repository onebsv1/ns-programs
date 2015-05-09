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
    std::cout<< Simulator::Now().GetSeconds() << "\t\t" << oldCwnd
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

    //uint32_t nFlows = 1;
    uint32_t segSize = 512;
    float start_time = 0.0;    

    /*double start[nFlows];

    //Getting all the start times from the random variable.
    for(unsigned int i = 0;i < nFlows; i++){
        start[i] = U->GetValue();
    }*/

    uint32_t maxBytes = 10000;

    // Configuration
    Config::SetDefault("ns3::TcpL4Protocol::SocketType", StringValue("ns3::TcpTahoe"));
    Config::SetDefault("ns3::TcpSocket::SegmentSize", UintegerValue (segSize));
    //GlobalValue::Bind ("ChecksumEnabled", BooleanValue (false));
    Config::SetDefault("ns3::TcpSocketBase::WindowScaling", BooleanValue (true));
    
    /*LogComponentEnable("TcpReno", LOG_LEVEL_INFO);
    LogComponentEnable("PacketSink", LOG_LEVEL_INFO);*/

    std::string animFile = "tahoereno-animation.xml" ;  // Name of file for animation output

    NodeContainer s1r1;
    s1r1.Create (2);

    // Setting up network topology
    
    PointToPointHelper Leaf;

    Leaf.SetDeviceAttribute("DataRate", StringValue("5Mbps"));
    Leaf.SetChannelAttribute("Delay", StringValue("10ms"));

    NetDeviceContainer nodeDevices1;
    nodeDevices1 = Leaf.Install(s1r1);

    InternetStackHelper stack;
    stack.Install(s1r1);

    Ipv4AddressHelper address;
    address.SetBase ("10.1.1.0", "255.255.255.0");
    Ipv4InterfaceContainer p2pInterfaces1;
    p2pInterfaces1 = address.Assign (nodeDevices1);


    
    Ipv4GlobalRoutingHelper::PopulateRoutingTables ();
    //In this simulation, the client sends to the server.
    // Setting up applications
    ApplicationContainer clientApps;
    ApplicationContainer serverApps;

    //TCP Clients
    
        //Calling the BulkSendHelper to setup a sender.
        BulkSendHelper clientHelper ("ns3::TcpSocketFactory",InetSocketAddress (p2pInterfaces1.GetAddress(1), 9));
        // Set the amount of data to send in bytes.  Zero is unlimited.
        clientHelper.SetAttribute ("MaxBytes", UintegerValue (maxBytes));
        clientApps = clientHelper.Install (s1r1.Get(0));

        // Setting up simulation times
        clientApps.Start (Seconds (0.0));
        clientApps.Stop (Seconds (5.0));

    std::list<uint32_t> dropList;
    // TCP Servers    
        
        Address sinkAddress (InetSocketAddress (Ipv4Address::GetAny(),9));
        //Calling the PacketSinkHelper to setup a receiver(sink).
        PacketSinkHelper serverHelper("ns3::TcpSocketFactory", sinkAddress);
        serverApps.Add(serverHelper.Install(s1r1.Get(1)));

        // Setting up simulation times
        serverApps.Start(Seconds (0.0));
        serverApps.Stop(Seconds(5.0));

    Simulator::Schedule(Seconds(start_time + 0.00001), &TraceCwnd);

    //dropList.push_back (3);
    /*dropList.push_back (4);
    dropList.push_back (5);*/

    /*dropList.push_back (33);
    dropList.push_back (44);
    dropList.push_back (55);
*/
    /*dropList.push_back (33);
    dropList.push_back (49);
    dropList.push_back (75);*/

    /*dropList.push_back (95);
    dropList.push_back (96);
    dropList.push_back (97);*/


    //Sally Floyd
    //dropList.push_back (5);
    dropList.push_back (7);    
    //dropList.push_back (28);
    dropList.push_back (15);



    Ptr<ReceiveListErrorModel> pem = CreateObject<ReceiveListErrorModel> ();
    pem->SetList (dropList);
    nodeDevices1.Get(1)->SetAttribute ("ReceiveErrorModel", PointerValue (pem));
    
    /*//Adding a netanim module
    Ptr<Node> n1 = s1r1.Get (0);
    AnimationInterface anim (animFile);
    anim.SetConstantPosition (n1, 100, 100);

    //Adding a netanim module
    Ptr<Node> n2 = s1r1.Get (1);
    anim.SetConstantPosition (n2, 100, 200);*/
    

    Simulator::Stop (Seconds (5.0));
    Simulator::Run();

    
    Ptr<PacketSink> sink1 = DynamicCast<PacketSink> (serverApps.Get (0));    
    cout << "flow "<<0<<" Goodput "<< (double)sink1->GetTotalRx () / (double) (5.0 /*- start[0]*/)  << std::endl;
    

    Simulator::Destroy();

    return 0;
}
