#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/applications-module.h"
#include "ns3/packet-sink.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/point-to-point-layout-module.h"
#include "ns3/random-variable.h"
#include "ns3/udp-client-server-helper.h"
#include "ns3/wifi-module.h"
#include "ns3/mobility-module.h"
#include "ns3/csma-module.h"
#include "ns3/internet-module.h"
#include "ns3/aodv-helper.h"
#include "ns3/olsr-helper.h"

#include <stdlib.h>
#include "string.h"
#include <sstream>

using namespace std;
using namespace ns3;

vector<bool> mid;
vector<uint32_t> sources;

void randomAssign(uint32_t nodes){

    
    int random;

    Ptr<UniformRandomVariable> U = CreateObject<UniformRandomVariable> ();
    U->SetAttribute ("Stream", IntegerValue (6110));
    U->SetAttribute ("Min", DoubleValue (0.0));
    U->SetAttribute ("Max", DoubleValue (nodes));

    for (uint32_t i = 0; i < nodes; ++i){
      mid.push_back(0);
    }

    for (uint32_t i = 0; i < nodes; ++i){
      sources.push_back(0);
    }

    mid[0] =1;                                          //a necessary hack, cannot use 0 value in sources array
                                                          //so we set spot 0 = 0 and start randomizing from 1

    for (uint32_t i = 1; i < nodes; ++i){
        
        while(sources[i] == 0){
          random = (int) U->GetValue();
          
          if(mid[random] != 1){           
            sources[i] = random;
            mid[random] = 1;
           
          }
          
        }
    }

}

int main(int argc, char **argv){


	  uint32_t      nodes 					= 20;  				    	//the number of WiFi nodes
  	float 		    trafficInt		  = 0.5;					    //traffic intensity
  	double   	    Ptx 						= 500; 					    //mW
  	string   	    rtalgo 					= "AODV";				    //routing algorithm
  	string        dRate           = "54Mbps";         //UDP source data rate
  	uint32_t      pktSize 				=  1500;            //packet size (802.11g default=1500)

  	//
  	//	Get the command line arguments, number of nodes, traffic intensity, transmit power, routing algorithm
  	//

  	CommandLine cmd;
	  cmd.AddValue ("nodes", 		"Number of wifi nodes",    		 nodes);
	  cmd.AddValue ("trafficInt", "Set trafficInt", 			     trafficInt);
	  cmd.AddValue ("Ptx", 		"Tx Power", 				  	 Ptx);
	  cmd.AddValue ("rtalgo", 	"Routing Algorithm", 			 rtalgo);
	  cmd.Parse (argc,argv);
	

    //
  	//	Setting the data rate of the nodes, using the 802.11g standard so, the data rate will be 54Mbps by default
  	//

  	double 		  dRateDec 					= 54.0 * trafficInt/nodes;			//calculating data rate (54Mbps x intensity/nodes)
	
	  ostringstream dRateStr;
	  dRateStr <<dRateDec<<"Mbps";
	  dRate = dRateStr.str();

	  //
  	//	Create WiFi nodes, channel and mobility
  	//

  	NodeContainer wiNodes;														//create Wifi nodes 
	  wiNodes.Create(nodes);
	  
	  YansWifiChannelHelper channel = YansWifiChannelHelper::Default ();			//creating the phy layer channel
	  YansWifiPhyHelper phyLayer    = YansWifiPhyHelper::Default ();
	  phyLayer.Set("TxPowerStart",    DoubleValue(10.0*log10(Ptx))); 				//dbm
	  phyLayer.Set("TxPowerEnd",      DoubleValue(10.0*log10(Ptx)));   			//dbm
	  phyLayer.SetChannel(channel.Create ());

	  WifiHelper wifi = WifiHelper::Default ();
	  wifi.SetStandard (WIFI_PHY_STANDARD_80211g);
	  wifi.SetRemoteStationManager ("ns3::ConstantRateWifiManager","DataMode", StringValue ("ErpOfdmRate6Mbps"),"ControlMode",StringValue ("ErpOfdmRate6Mbps")); 

	  NqosWifiMacHelper wiMac = NqosWifiMacHelper::Default ();					//WiFi MAC helper without QOS
	  
	  NetDeviceContainer wiDevices;												//Install WiFi devices on the nodes
	  wiDevices = wifi.Install (phyLayer, wiMac, wiNodes);						//Install WiFi channel on the nodes

	  MobilityHelper mobility;

	  //Install mobile nodes and set random positions for the nodes in a 1000m x 1000m grid

	  mobility.SetPositionAllocator ("ns3::RandomRectanglePositionAllocator",		
	                                   "X", StringValue ("ns3::UniformRandomVariable[Min=0.0|Max=1000.0]"),
	                                   "Y", StringValue ("ns3::UniformRandomVariable[Min=0.0|Max=1000.0]"));


	  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
	  mobility.Install (wiNodes);

	  InternetStackHelper stack;

	  //
	  // Assign a routing algorithm to the nodes, either AODV or OLSR
	  //
  
	  if(rtalgo == "AODV"){
	    AodvHelper aodv;
	    stack.SetRoutingHelper(aodv);
	  }else{
	    OlsrHelper olsr;
	    stack.SetRoutingHelper(olsr);
	  }

	  stack.Install (wiNodes);
  
  	//
  	// Assign a IP Addresses to the nodes
	  //

	  Ipv4AddressHelper address;
	  address.SetBase ("10.1.0.0", "255.255.0.0");

	  Ipv4InterfaceContainer wiInterfaces;
	  wiInterfaces =   address.Assign (wiDevices);

	  //
  	// Install On/Off applications on all the nodes
  	//

	  DataRate x(dRate);
  	OnOffHelper clientHelper  ("ns3::UdpSocketFactory", Address ());
  	clientHelper.SetAttribute ("OnTime", StringValue ("ns3::UniformRandomVariable[Min=0.,Max=1.]"));
  	clientHelper.SetAttribute ("OffTime", StringValue ("ns3::UniformRandomVariable[Min=0.,Max=0.]"));
  	clientHelper.SetConstantRate(x,pktSize);
  	ApplicationContainer clientApps;

    randomAssign(nodes);

  	uint16_t port = 9;													//assign a random port

  	for (uint32_t i = 0; i < nodes; ++i){
  	    AddressValue remoteAddress (InetSocketAddress (wiInterfaces.GetAddress (sources[i]), port));
  	    clientHelper.SetAttribute ("Remote", remoteAddress);
  	    clientApps.Add (clientHelper.Install (wiNodes.Get (i)));
  	}

  	clientApps.Start (Seconds (0.0));
  	clientApps.Stop (Seconds (10.0));

  
  	Address sinkLocalAddress (InetSocketAddress (Ipv4Address::GetAny (), port));
  	PacketSinkHelper packetSinkHelper ("ns3::UdpSocketFactory", sinkLocalAddress);
  	ApplicationContainer sinkApps; 

  	for (uint32_t i = 0; i < nodes; ++i){
      sinkApps.Add (packetSinkHelper.Install (wiNodes.Get(i)));
    }
  	
  	sinkApps.Start (Seconds (0.0));  	
  	sinkApps.Stop (Seconds (10.0));

  	Ipv4GlobalRoutingHelper::PopulateRoutingTables ();
  	Simulator::Stop (Seconds (11.0));


  	Simulator::Run ();

  	double totalRx = 0.0;
  	double totalTx = 0.0;

  	for (uint32_t i = 0; i < sinkApps.GetN(); i++){
    	Ptr <Application> app = sinkApps.Get (i);
    	Ptr <PacketSink> pktSink = DynamicCast <PacketSink>(app);
    	totalRx += pktSink->GetTotalRx ();
    }

  	for (uint32_t i = 0; i < clientApps.GetN(); i++){
    	Ptr <Application> app = clientApps.Get (i);
    	Ptr <OnOffApplication> onOffApp = DynamicCast <OnOffApplication> (app);
    	totalTx += onOffApp->GetTotBytes();
    }
  
  	
  	cout<<"Output\t"<<nodes<<"\t"<<Ptx<<"\t"<<trafficInt<<"\t"<<rtalgo.c_str()<<"\t"<<(float) totalRx<<(float)totalTx<<endl;  
  
  	Simulator::Destroy ();

	  return 0;
}