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
#include "ns3/point-to-point-module.h"
#include "ns3/network-module.h"
#include "ns3/applications-module.h"
#include "ns3/mobility-module.h"
#include "ns3/csma-module.h"
#include "ns3/internet-module.h"
#include "ns3/sixlowpan-module.h"
#include "ns3/ipv6-list-routing-helper.h"
#include "ns3/ssid.h"

// Default Network Topology
//
//      Wifi 2001:1::
//    AP
//    *    *    *    *
//    |    |    |    |   
//   n0   n1   n2   n3 
// client          server 

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("EPRI_AMI");

class AmiExample
{
  public:
    AmiExample() = default;
    /**
     * Run function
     * \param nWifi The number of nodes
     * \param areaWidth The width of the simulation area in meters
     * \param areaHeight The height of the simulation area in meters
     * \param verbose Tell echo applications to log if true
     * \param tracing Enable pcap tracing
     */
    void CaseRun(uint32_t nWifi, 
                 unsigned areaWidth,
                 unsigned areaHeight,
                 bool verbose,
                 bool tracing);
  private:
    /// Create and initialize all nodes
    NodeContainer CreateNodes(uint32_t nWifi);
    /// Create a fixed mobility helper within the area
    MobilityHelper CreateMobility(unsigned areaWidth, unsigned areaHeight);
};

int 
main(int argc, char *argv[])
{
  bool verbose = true;
  uint32_t nWifi = 3;
  bool tracing = false;
  unsigned areaWidth = 50;
  unsigned areaHeight = 50;

  CommandLine cmd(__FILE__);
  cmd.AddValue("nWifi", "Number of wifi STA devices", nWifi);
  cmd.AddValue("verbose", "Tell echo applications to log if true", verbose);
  cmd.AddValue("tracing", "Enable pcap tracing", tracing);
  cmd.AddValue("areaWidth", "Set width of simulation area in meters", areaWidth);
  cmd.AddValue("areaHeight", "Set height of simulation area in meters", areaHeight);

  cmd.Parse(argc,argv);

  if(verbose)
    {
      LogComponentEnable("UdpEchoClientApplication", LOG_LEVEL_INFO);
      LogComponentEnable("UdpEchoServerApplication", LOG_LEVEL_INFO);
    }
  AmiExample test = AmiExample();
  test.CaseRun(nWifi, 
               areaWidth,
               areaHeight,
               verbose,
               tracing);
}

void
AmiExample::CaseRun(uint32_t nWifi,
                    unsigned areaWidth,
                    unsigned areaHeight,
                    bool verbose,
                    bool tracing)
{
  NodeContainer wifiStaNodes = CreateNodes(nWifi);
  NodeContainer wifiApNode = wifiStaNodes.Get(0);

#if 0
  YansWifiChannelHelper channel = YansWifiChannelHelper::Default();
  YansWifiPhyHelper phy;
  phy.SetChannel(channel.Create());

  WifiMacHelper mac;
  Ssid ssid = Ssid("ns-3-ssid");

  WifiHelper wifi;

  NetDeviceContainer staDevices;
  mac.SetType("ns3::StaWifiMac",
               "Ssid", SsidValue(ssid),
               "ActiveProbing", BooleanValue(false));
  staDevices = wifi.Install(phy, mac, wifiStaNodes);

  NetDeviceContainer apDevices;
  mac.SetType("ns3::ApWifiMac"
               "Ssid", SsidValue(ssid));
  apDevices = wifi.Install(phy, mac, wifiApNode);
#else
  // create channels
  NS_LOG_INFO("Create channels.");
  CsmaHelper csma;
  csma.SetChannelAttribute("DataRate", DataRateValue(5000000));
  csma.SetChannelAttribute("Delay", TimeValue(MilliSeconds(2)));
  NetDeviceContainer apDevices = csma.Install(wifiApNode);
  csma.SetDeviceAttribute("Mtu", UintegerValue(150));
  NetDeviceContainer staDevices = csma.Install(wifiStaNodes);

  SixLowPanHelper sixlowpan;
  sixlowpan.SetDeviceAttribute("ForceEtherType", BooleanValue(true));
  staDevices = sixlowpan.Install(staDevices);

#endif

  MobilityHelper mobility = CreateMobility(areaWidth, areaHeight);
  mobility.Install(wifiStaNodes);

  Ipv6ListRoutingHelper listRh;
  Ipv6StaticRoutingHelper staticRh;
  listRh.Add(staticRh, 1);

  InternetStackHelper stack;
  stack.SetIpv4StackInstall(false); // IPv6 only
  stack.SetRoutingHelper(listRh);
  stack.Install(wifiStaNodes);

  Ipv6AddressHelper address;
  address.SetBase(Ipv6Address("2001:1::"), Ipv6Prefix(64));

  Ipv6InterfaceContainer staInterfaces = address.Assign(staDevices);
  address.Assign(apDevices);

  UdpEchoServerHelper echoServer(9);

  ApplicationContainer serverApps = echoServer.Install(wifiStaNodes.Get(0));
  serverApps.Start(Seconds(1.0));
  serverApps.Stop(Seconds(10.0));

  UdpEchoClientHelper echoClient(staInterfaces.GetAddress(0, 1), 9);
  echoClient.SetAttribute("MaxPackets", UintegerValue(1));
  echoClient.SetAttribute("Interval", TimeValue(Seconds(1.0)));
  echoClient.SetAttribute("PacketSize", UintegerValue(1024));

  ApplicationContainer clientApps = echoClient.Install(wifiApNode);
  clientApps.Start(Seconds(2.0));
  clientApps.Stop(Seconds(10.0));

  Simulator::Stop(Seconds(10.0));

  if(tracing)
    {
      csma.EnablePcap("ami", apDevices.Get(0));
    }

  Simulator::Run();
  Simulator::Destroy();
}

NodeContainer
AmiExample::CreateNodes(uint32_t nWifi)
{
  NodeContainer wifiStaNodes;
  wifiStaNodes.Create(nWifi);
  return wifiStaNodes;
}

MobilityHelper
AmiExample::CreateMobility(unsigned areaWidth, unsigned areaHeight)
{
  MobilityHelper mobility;
  std::ostringstream widthStream;
  widthStream << "ns3::UniformRandomVariable[Min=0|Max=" << areaWidth << "]";
  std::ostringstream heightStream;
  heightStream << "ns3::UniformRandomVariable[Min=0|Max=" << areaHeight << "]";
  mobility.SetPositionAllocator("ns3::RandomRectanglePositionAllocator",
                                "X", StringValue(widthStream.str()),
                                "Y", StringValue(heightStream.str())
                                );
  mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
  return mobility;
}
