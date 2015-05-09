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

NS_LOG_COMPONENT_DEFINE ("TcpExample");


int main(int argc, char **argv)
{

    RngSeedManager::SetSeed (11223344);
    Ptr<UniformRandomVariable> U = CreateObject<UniformRandomVariable> ();
    U->SetAttribute ("Stream", IntegerValue (6110));
    U->SetAttribute ("Min", DoubleValue (0.0));
    U->SetAttribute ("Max", DoubleValue (0.1));

    double start = U->GetValue();

    cout<<"Start time: "<<start<<endl;

    uint32_t maxBytes = 0;
    // Options
    GlobalValue::Bind ("ChecksumEnabled", BooleanValue (false));
    Config::SetDefault("ns3::TcpL4Protocol::SocketType", StringValue("ns3::TcpTahoe"));
    Config::SetDefault("ns3::TcpSocket::SegmentSize", UintegerValue (512));
    Config::SetDefault("ns3::TcpSocketBase::MaxWindowSize", UintegerValue (2000));
    Config::SetDefault("ns3::TcpSocketBase::WindowScaling", BooleanValue (false));
    
        
    //LogComponentEnable("OnOffApplication", LOG_LEVEL_INFO);
    LogComponentEnable("TcpSocketBase", LOG_LEVEL_INFO);
    LogComponentEnable("PointToPointDumbbellHelper", LOG_LEVEL_INFO);

    std::string animFile = "dumbbell-animation.xml" ;  // Name of file for animation output

    //
    // Setting up network topology
    // It is a dumbbell network with two leaves on each side
    PointToPointHelper PTPRouter;
    PointToPointHelper PTPLeaf;

    PTPRouter.SetDeviceAttribute("DataRate", StringValue("1Mbps"));
    PTPRouter.SetChannelAttribute("Delay", StringValue("20ms"));
    PTPRouter.SetQueue("ns3::DropTailQueue","MaxBytes",UintegerValue(64000));
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
    ApplicationContainer clientApps;
    ApplicationContainer serverApps;

    // Just one TCP connection for now

    BulkSendHelper clientHelper ("ns3::TcpSocketFactory",
                         InetSocketAddress (dumbbell.GetRightIpv4Address(0), 50000));
    // Set the amount of data to send in bytes.  Zero is unlimited.
    clientHelper.SetAttribute ("MaxBytes", UintegerValue (maxBytes));
    // Set the segment size
    //clientHelper.SetAttribute ("SendSize", UintegerValue (512));
    ApplicationContainer sourceApps = clientHelper.Install (dumbbell.GetLeft(0));
    sourceApps.Start (Seconds (start));
    sourceApps.Stop (Seconds (10.0));

    // TCP Servers
    PacketSinkHelper serverHelper("ns3::TcpSocketFactory", Address());

    AddressValue local(InetSocketAddress(dumbbell.GetRightIpv4Address(0), 50000));
    serverHelper.SetAttribute("Local", local);
    serverApps.Add(serverHelper.Install(dumbbell.GetRight(0)));

    //
    // Setting up simulation
    serverApps.Start(Seconds (start));
    serverApps.Stop(Seconds(10.0));
    clientApps.Start(Seconds (start));
    clientApps.Stop(Seconds(10.0));

    dumbbell.BoundingBox (1, 1, 100, 100);
    AnimationInterface anim (animFile);
    anim.EnablePacketMetadata (); // Optional
    anim.EnableIpv4L3ProtocolCounters (Seconds (0), Seconds (10)); // Optional
    
    Ptr<PacketSink> sink1 = DynamicCast<PacketSink> (serverApps.Get (0));
   

    PTPRouter.EnablePcapAll("TahoeCongestion");
    PTPLeaf.EnablePcapAll("TahoeCongestion");

    Simulator::Stop (Seconds (10.0));
    Simulator::Run();
    std::cout << "Animation Trace file created:" << animFile.c_str ()<< std::endl;
    std::cout << "Total Bytes Received: " << sink1->GetTotalRx () << std::endl;
    std::cout << "Goodput: " << sink1->GetTotalRx () / (10.0 - start)  << std::endl;
    Simulator::Destroy();

    cout << "Done." << endl;

    return 0;
}
