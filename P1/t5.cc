#include <iostream>

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/point-to-point-layout-module.h"

// Network topology
//
//       n0 ------------ n1-------n2-------------n3
//            5 Mbps        1 Mbps   5 Mbps
//             10 ms        20 ms     10 ms
//
// - Flow from n0 to n3 using BulkSendApplication.



using namespace ns3;
using namespace std;


int main(int argc, char **argv)
{

    RngSeedManager::SetSeed (11223344);
    Ptr<UniformRandomVariable> U = CreateObject<UniformRandomVariable> ();
    U->SetAttribute ("Stream", IntegerValue (6110));
    U->SetAttribute ("Min", DoubleValue (0.0));
    U->SetAttribute ("Max", DoubleValue (0.1));

    uint32_t nFlows = 10;
    uint32_t queueSize = 64000;
    uint32_t windowSize = 2000;
    uint32_t segSize = 512;
    

    CommandLine cmd;
    cmd.AddValue("queueSize","queue size",queueSize);
    cmd.AddValue("windowSize","window size",windowSize);
    cmd.AddValue("segSize","segment size",segSize);
    cmd.AddValue("nFlows","nflows",nFlows);
    cmd.Parse(argc,argv);

    double start[nFlows];

    //Getting all the start times from the random variable.
    for(unsigned int i = 0;i < nFlows; i++){
        start[i] = U->GetValue();
    }

    uint32_t maxBytes = 0;

    // Configuration
    GlobalValue::Bind ("ChecksumEnabled", BooleanValue (false));
    Config::SetDefault("ns3::TcpL4Protocol::SocketType", StringValue("ns3::TcpTahoe"));
    Config::SetDefault("ns3::TcpSocket::SegmentSize", UintegerValue (segSize));
    Config::SetDefault("ns3::TcpSocketBase::MaxWindowSize", UintegerValue (windowSize));
    Config::SetDefault("ns3::TcpSocketBase::WindowScaling", BooleanValue (false));
    
    LogComponentEnable("TcpSocketBase", LOG_LEVEL_INFO);
    LogComponentEnable("PointToPointDumbbellHelper", LOG_LEVEL_INFO);

    

    // Setting up network topology
    // It is a dumbbell network with nFlows leaves on each side
    PointToPointHelper Router;
    PointToPointHelper Leaf;

    Router.SetDeviceAttribute("DataRate", StringValue("1Mbps"));
    Router.SetChannelAttribute("Delay", StringValue("20ms"));
    Router.SetQueue("ns3::DropTailQueue","MaxBytes",UintegerValue(queueSize));
    Router.SetQueue("ns3::DropTailQueue","Mode",StringValue("QUEUE_MODE_BYTES"));

    Leaf.SetDeviceAttribute("DataRate", StringValue("5Mbps"));
    Leaf.SetChannelAttribute("Delay", StringValue("10ms"));

    PointToPointDumbbellHelper dumbbell(nFlows, Leaf, nFlows, Leaf, Router);
    
    InternetStackHelper stack;
    dumbbell.InstallStack(stack);

    dumbbell.AssignIpv4Addresses(Ipv4AddressHelper("10.0.1.0", "255.255.255.0"),
                                 Ipv4AddressHelper("10.2.1.0", "255.255.255.0"),
                                 Ipv4AddressHelper("10.1.1.0", "255.255.255.0"));
    
   
    //Populate the routing tables.
    Ipv4GlobalRoutingHelper::PopulateRoutingTables ();   
    

    //In this simulation, the client sends to the server.
    // Setting up applications
    ApplicationContainer clientApps[nFlows];
    ApplicationContainer serverApps[nFlows];

    //TCP Clients
    for(unsigned int i = 0;i < nFlows; i++){
        //Calling the BulkSendHelper to setup a sender.
        BulkSendHelper clientHelper ("ns3::TcpSocketFactory",InetSocketAddress (dumbbell.GetRightIpv4Address(i), 9));
        // Set the amount of data to send in bytes.  Zero is unlimited.
        clientHelper.SetAttribute ("MaxBytes", UintegerValue (maxBytes));
        clientApps[i] = clientHelper.Install (dumbbell.GetLeft(i));

        // Setting up simulation times
        clientApps[i].Start (Seconds (start[i]));
        clientApps[i].Stop (Seconds (10.0));
    }

    // TCP Servers    
    for(unsigned int i = 0;i < nFlows; i++){
        //Calling the PacketSinkHelper to setup a receiver(sink).
        PacketSinkHelper serverHelper("ns3::TcpSocketFactory", InetSocketAddress (Ipv4Address::GetAny(),9));

        AddressValue local(InetSocketAddress(dumbbell.GetRightIpv4Address(i), 9));
        serverHelper.SetAttribute("Local", local);
        serverApps[i].Add(serverHelper.Install(dumbbell.GetRight(i)));

        // Setting up simulation times
        serverApps[i].Start(Seconds (start[i]));
        serverApps[i].Stop(Seconds(10.0));
    }
   

    Simulator::Stop (Seconds (10.0));
    Simulator::Run();

    for(unsigned int i = 0;i < nFlows; i++){
        //Creating a sink object.
        Ptr<PacketSink> sink1 = DynamicCast<PacketSink> (serverApps[i].Get (0));    
        cout << "flow "<<i<<" windowSize "<<windowSize<<" queueSize "<<queueSize<<" Goodput "<< (double)sink1->GetTotalRx () / (double) (10.0 - start[i])  << std::endl;
    }

    Simulator::Destroy();

    return 0;
}
