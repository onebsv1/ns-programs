#include <iostream>

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/netanim-module.h"
#include "ns3/applications-module.h"
#include "ns3/point-to-point-layout-module.h"

// Network topology
//
//       n0 ------------ n1-------n2--------n3
//            5 Mbps        1 Mbps   5 Mbps
//             10 ms        20 ms     10 ms
//
// - Flow from n0 to n3 using OnOffApplication.



using namespace ns3;
using namespace std;

//NS_LOG_COMPONENT_DEFINE ("TcpExample");


int main(int argc, char **argv)
{

    RngSeedManager::SetSeed (11223344);
    Ptr<UniformRandomVariable> U = CreateObject<UniformRandomVariable> ();
    U->SetAttribute ("Stream", IntegerValue (6110));
    U->SetAttribute ("Min", DoubleValue (0.0));
    U->SetAttribute ("Max", DoubleValue (0.1));

    uint32_t nFlows = 5;
    

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

    for(unsigned int i = 0;i < nFlows; i++){
        start[i] = U->GetValue();
        cout<<"Start time: "<<start[i]<<endl;
    }

    uint32_t maxBytes = 0;
    // Options
    GlobalValue::Bind ("ChecksumEnabled", BooleanValue (false));
    Config::SetDefault("ns3::TcpL4Protocol::SocketType", StringValue("ns3::TcpTahoe"));
    Config::SetDefault("ns3::TcpSocket::SegmentSize", UintegerValue (segSize));
    Config::SetDefault("ns3::TcpSocketBase::MaxWindowSize", UintegerValue (windowSize));
    Config::SetDefault("ns3::TcpSocketBase::WindowScaling", BooleanValue (false));
    
        
    //LogComponentEnable("OnOffApplication", LOG_LEVEL_INFO);
    //LogComponentEnable("TcpSocketBase", LOG_LEVEL_INFO);
    //LogComponentEnable("PointToPointDumbbellHelper", LOG_LEVEL_INFO);

    std::string animFile = "P1.xml" ;  // Name of file for animation output

    //
    // Setting up network topology
    // It is a dumbbell network with two leaves on each side
    PointToPointHelper PTPRouter;
    PointToPointHelper PTPLeaf;

    PTPRouter.SetDeviceAttribute("DataRate", StringValue("1Mbps"));
    PTPRouter.SetChannelAttribute("Delay", StringValue("20ms"));
    PTPRouter.SetQueue("ns3::DropTailQueue","MaxBytes",UintegerValue(queueSize));
    PTPRouter.SetQueue("ns3::DropTailQueue","Mode",StringValue("QUEUE_MODE_BYTES"));

    PTPLeaf.SetDeviceAttribute("DataRate", StringValue("5Mbps"));
    PTPLeaf.SetChannelAttribute("Delay", StringValue("10ms"));

    PointToPointDumbbellHelper dumbbell(1, PTPLeaf, 1, PTPLeaf, PTPRouter);
    
    InternetStackHelper stack;
    dumbbell.InstallStack(stack);

    dumbbell.AssignIpv4Addresses(Ipv4AddressHelper("10.0.1.0", "255.255.255.0"),
                                      Ipv4AddressHelper("10.2.1.0", "255.255.255.0"),
                                      Ipv4AddressHelper("10.1.1.0", "255.255.255.0"));
    
   
   //Populate the routing tables.
    Ipv4GlobalRoutingHelper::PopulateRoutingTables ();
    
    /*cout << "Left leaves: " << endl;
    cout << "\tSink: " << dumbbell.GetLeftIpv4Address(0) << endl;
    
    cout << "Right leaves: " << endl;
    cout << "\tSource: " << dumbbell.GetRightIpv4Address(0) << endl;*/
   
    

    //
    // Setting up applications
    ApplicationContainer clientApps[nFlows];
    ApplicationContainer serverApps[nFlows];

    // Just one TCP connection for now

   
    
   

    for(unsigned int i = 0;i < nFlows; i++){
     BulkSendHelper clientHelper ("ns3::TcpSocketFactory",
                         InetSocketAddress (dumbbell.GetRightIpv4Address(0), 50000+i));
        // Set the amount of data to send in bytes.  Zero is unlimited.
        clientHelper.SetAttribute ("MaxBytes", UintegerValue (maxBytes));
        clientApps[i] = clientHelper.Install (dumbbell.GetLeft(0));
        clientApps[i].Start (Seconds (start[i]));
        clientApps[i].Stop (Seconds (10.0));
    }

    
    


    for(unsigned int i = 0;i < nFlows; i++){
        // TCP Servers
         PacketSinkHelper serverHelper("ns3::TcpSocketFactory", Address());

        AddressValue local(InetSocketAddress(dumbbell.GetRightIpv4Address(0), 50000+i));
        serverHelper.SetAttribute("Local", local);
        serverApps[i].Add(serverHelper.Install(dumbbell.GetRight(0)));

        //
        // Setting up simulation
        serverApps[i].Start(Seconds (start[i]));
        serverApps[i].Stop(Seconds(10.0));
    }
    

    dumbbell.BoundingBox (1, 1, 100, 100);
    AnimationInterface anim (animFile);
    anim.EnablePacketMetadata (); // Optional
    anim.EnableIpv4L3ProtocolCounters (Seconds (0), Seconds (10)); // Optional
   

    PTPRouter.EnablePcapAll("TahoeCongestion");
    PTPLeaf.EnablePcapAll("TahoeCongestion");

    Simulator::Stop (Seconds (10.0));
    Simulator::Run();
    std::cout << "Animation Trace file created:" << animFile.c_str ()<< std::endl;

    for(unsigned int i = 0;i < nFlows; i++){
        Ptr<PacketSink> sink1 = DynamicCast<PacketSink> (serverApps[i].Get (0));    
        std::cout << "Goodput "<<i<<" : "<< sink1->GetTotalRx () / (10.0 - start[i])  << std::endl;
    }
    Simulator::Destroy();

    cout << "Done." << endl;

    return 0;
}
