/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "ns3/core-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-layout-module.h"
#include "ns3/applications-module.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("ScratchSimulator");

int 
main (int argc, char *argv[])
{
  SeedManager::SetSeed(1);

  uint32_t nSpokes = 8;
  double endTime = 60.0;
  std::string protocol = "TcpHybla";

  //Process the command line

  CommandLine cmd;
  cmd.AddValue("nSpokes", "Number of spokes to place in each star", nSpokes);
  cmd.AddValue("Protocol", "TCP Protocol to use: ", protocol);
  cmd.Parse(argc, argv);

  if(protocol == "TcpNewReno") {
    Config::SetDefault("ns3::TcpL4Protocol::SocketType", TypeIdValue(TcpNewReno::GetTypeId()));
  } else if (protocol == "TcpHybla") {
    Config::SetDefault("ns3::TcpL4Protocol::SocketType", TypeIdValue(TcpHybla::GetTypeId()));
  } else if (protocol == "TcpHighSpeed") {
    Config::SetDefault("ns3::TcpL4Protocol::SocketType", TypeIdValue(TcpHighSpeed::GetTypeId()));
  } 
  else if (protocol == "TcpHtcp") {
    Config::SetDefault("ns3::TcpL4Protocol::SocketType", TypeIdValue(TcpHtcp::GetTypeId()));
  } else if (protocol == "TcpVegas") {
    Config::SetDefault("ns3::TcpL4Protocol::SocketType", TypeIdValue(TcpVegas::GetTypeId()));
  } 
  else if (protocol == "TcpScalable") {
    Config::SetDefault("ns3::TcpL4Protocol::SocketType", TypeIdValue(TcpScalable::GetTypeId()));
  } else if (protocol == "TcpVeno") {
    Config::SetDefault("ns3::TcpL4Protocol::SocketType", TypeIdValue(TcpVeno::GetTypeId()));
  } 
  else if (protocol == "TcpBic") {
    Config::SetDefault("ns3::TcpL4Protocol::SocketType", TypeIdValue(TcpBic::GetTypeId()));
  } else if (protocol == "TcpYeah") {
    Config::SetDefault("ns3::TcpL4Protocol::SocketType", TypeIdValue(TcpYeah::GetTypeId()));
  } 
  else if (protocol == "TcpIllinois") {
    Config::SetDefault("ns3::TcpL4Protocol::SocketType", TypeIdValue(TcpIllinois::GetTypeId()));
  } 
  else if (protocol == "TcpWestwood") {
    Config::SetDefault("ns3::TcpL4Protocol::SocketType", TypeIdValue(TcpWestwood::GetTypeId()));
  }
  else if (protocol == "TcpWestwoodPlus") {
    Config::SetDefault("ns3::TcpL4Protocol::SocketType", TypeIdValue(TcpWestwood::GetTypeId()));
  }


  //Use the given p2p attributes for each star
  PointToPointHelper pointToPoint;
  pointToPoint.SetDeviceAttribute("DataRate", StringValue("5Mbps"));
  pointToPoint.SetChannelAttribute("Delay", StringValue("10ms"));
  
  //Use the pointToPoint helper to create point to point stars
  PointToPointStarHelper star(nSpokes, pointToPoint);
  PointToPointStarHelper star2(nSpokes, pointToPoint);

  InternetStackHelper internet;
  //install internet stack on all nodes of star1, including hub node
  star.InstallStack(internet);
  star2.InstallStack(internet);

  //connect the two stars via their hubs
  //the PointToPointHelper used to create a link between the stars' hubs
  PointToPointHelper p2pHelper;
  p2pHelper.SetDeviceAttribute ("DataRate", StringValue ("1Mbps"));
  p2pHelper.SetChannelAttribute ("Delay", StringValue ("20ms"));

  NetDeviceContainer p2pDevices;
  //create a link between the two stars' hub nodes
  p2pDevices = p2pHelper.Install(star.GetHub(), star2.GetHub());

  Ipv4AddressHelper address("10.1.1.0", "255.255.255.0");
  star.AssignIpv4Addresses(address);
  address.SetBase("10.2.1.0", "255.255.255.0");
  star2.AssignIpv4Addresses(address);
  address.SetBase("10.3.1.0", "255.255.255.0");
  address.Assign(p2pDevices);

  //Use the PacketSinkHelper to install an instance of a packet sink on 
  //each of the spokes on the receiving star. 
  //let star receive data
  Address sinkLocalAddress(InetSocketAddress(Ipv4Address::GetAny(), 5000));
  PacketSinkHelper sinkHelper("ns3::TcpSocketFactory", sinkLocalAddress);
  ApplicationContainer sinkApps;
  for(uint32_t i = 0; i < nSpokes; ++i) {
  	sinkApps.Add(sinkHelper.Install(star.GetSpokeNode(i)));
  }

  sinkApps.Start(Seconds(1.0));

  //Install an instance of BulkSendApplication on each of the sending nodes.
  //let star2 send data
  BulkSendHelper bulkSender("ns3::TcpSocketFactory", Address());
  //bulkSender.SetAttribute("SendSize", UintegerValue(1024));

  ApplicationContainer sourceApps;
  for(uint32_t i = 0; i < nSpokes; ++i) 
  {
    //uint32_t remoteNode = (i + 1) % nSpokes;
    uint32_t remoteNode = i;
  	AddressValue remoteAddress(InetSocketAddress(star.GetSpokeIpv4Address(remoteNode), 5000));
  	bulkSender.SetAttribute("Remote", remoteAddress);
  	sourceApps.Add(bulkSender.Install(star2.GetSpokeNode(i)));
  }

  sourceApps.Start(Seconds(2.0));

  Ipv4GlobalRoutingHelper::PopulateRoutingTables();

  Simulator::Stop(Seconds(endTime));

  Simulator::Run ();

  uint64_t totalRx = 0;

  for(uint32_t i = 0; i < nSpokes; ++i) {

  	Ptr<PacketSink> sink = DynamicCast<PacketSink>(sinkApps.Get(i));
  	uint32_t bytesReceived = sink->GetTotalRx();
  	totalRx += bytesReceived;
  	std::cout << "Sink " << i << "\tTotalRx: " << bytesReceived * 1e-6 * 8 << "Mb";
  	std::cout << "\tThroughput: " << (bytesReceived * 1e-6 * 8) / endTime << "Mbps" << std::endl;
  }

  std::cout << std::endl;
  std::cout << "Totals\tTotalRx: " << totalRx * 1e-6 * 8 << "Mb";
  std::cout << "\tThroughput: " << (totalRx * 1e-6 * 8) / endTime << "Mbps" << std::endl;

  Simulator::Destroy ();
}
