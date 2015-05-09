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

// Network topology
//
//       n0 ------------ n1-------------n2
//            5 Mbps          5 Mbps
//             10 ms           10 ms
//
// - Flow from n0 to n2 using BulkSendApplication.



using namespace ns3;
using namespace std;


int main(int argc, char **argv)
{

    RngSeedManager::SetSeed (11223344);
    Ptr<UniformRandomVariable> U = CreateObject<UniformRandomVariable> ();
    U->SetAttribute ("Stream", IntegerValue (6110));
    U->SetAttribute ("Min", DoubleValue (0.0));
    U->SetAttribute ("Max", DoubleValue (0.1));

    uint32_t nFlows = 1;
    uint32_t segSize = 512;
    

    double start[nFlows];

    //Getting all the start times from the random variable.
    for(unsigned int i = 0;i < nFlows; i++){
        start[i] = U->GetValue();
    }

    uint32_t maxBytes = 2000;

    // Configuration
    Config::SetDefault("ns3::TcpL4Protocol::SocketType", StringValue("ns3::TcpTahoe"));
    Config::SetDefault("ns3::TcpSocket::SegmentSize", UintegerValue (segSize));
    Config::SetDefault ("ns3::TcpSocket::DelAckCount", UintegerValue (1));
    
    LogComponentEnable("TcpTahoe", LOG_LEVEL_INFO);
    LogComponentEnable("PacketSink", LOG_LEVEL_INFO);


    NodeContainer s1m1r1;
    s1m1r1.Create (3);
    

    NodeContainer s1m1 (s1m1r1.Get(0), s1m1r1.Get(1));
    NodeContainer m1r1 (s1m1r1.Get(1), s1m1r1.Get(2));


    // Setting up network topology
    
    /*PointToPointHelper Router;*/
    PointToPointHelper Leaf;

    /*Router.SetDeviceAttribute("DataRate", StringValue("1Mbps"));
    Router.SetChannelAttribute("Delay", StringValue("20ms"));
    Router.SetQueue("ns3::DropTailQueue","MaxBytes",UintegerValue(queueSize));
    Router.SetQueue("ns3::DropTailQueue","Mode",StringValue("QUEUE_MODE_BYTES"));*/

    Leaf.SetDeviceAttribute("DataRate", StringValue("5Mbps"));
    Leaf.SetChannelAttribute("Delay", StringValue("10ms"));

    NetDeviceContainer nodeDevices1;
    nodeDevices1 = Leaf.Install(s1m1);

    NetDeviceContainer nodeDevices2;
    nodeDevices2 = Leaf.Install (m1r1);

    InternetStackHelper stack;
    stack.Install(s1m1r1);

    Ipv4AddressHelper address;
    address.SetBase ("10.1.1.0", "255.255.255.0");
    Ipv4InterfaceContainer p2pInterfaces1;
    p2pInterfaces1 = address.Assign (nodeDevices1);

    Ipv4AddressHelper address1;
    address1.SetBase ("10.1.2.0", "255.255.255.0");
    Ipv4InterfaceContainer p2pInterfaces2;
    p2pInterfaces2 = address1.Assign (nodeDevices2);

    
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
        clientApps = clientHelper.Install (s1m1r1.Get(0));

        // Setting up simulation times
        clientApps.Start (Seconds (start[0]));
        clientApps.Stop (Seconds (10.0));

    std::list<uint32_t> dropList;
    // TCP Servers    
    
        //Calling the PacketSinkHelper to setup a receiver(sink).
        PacketSinkHelper serverHelper("ns3::TcpSocketFactory", InetSocketAddress (Ipv4Address::GetAny(),9));
        serverApps.Add(serverHelper.Install(s1m1r1.Get(2)));

        // Setting up simulation times
        serverApps.Start(Seconds (start[0]));
        serverApps.Stop(Seconds(10.0));

    
    dropList.push_back (1);
    dropList.push_back (2);    
    dropList.push_back (3);
    dropList.push_back (4);

    Ptr<ReceiveListErrorModel> pem = CreateObject<ReceiveListErrorModel> ();
    pem->SetList (dropList);
    nodeDevices2.Get(1)->SetAttribute ("ReceiveErrorModel", PointerValue (pem));
      

    Simulator::Stop (Seconds (10.0));
    Simulator::Run();

    
    Ptr<PacketSink> sink1 = DynamicCast<PacketSink> (serverApps.Get (0));    
    cout << "flow "<<0<<" Goodput "<< (double)sink1->GetTotalRx () / (double) (10.0 - start[0])  << std::endl;
    

    Simulator::Destroy();

    return 0;
}