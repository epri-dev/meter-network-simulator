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
#include "ns3/internet-module.h"
#include "ns3/lr-wpan-module.h"
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
     * \param nAmiNodes The number of nodes
     * \param areaWidth The width of the simulation area in meters
     * \param areaHeight The height of the simulation area in meters
     * \param verbose Tell echo applications to log if true
     * \param tracing Enable pcap tracing
     */
    void CaseRun(uint32_t nAmiNodes, 
                 unsigned areaWidth,
                 unsigned areaHeight,
                 double durationSeconds,
                 bool useMeshUnder,
                 bool verbose,
                 bool tracing);
  private:
    /// Create and initialize all nodes
    NodeContainer CreateNodes(uint32_t nAmiNodes);
    /// Create a fixed mobility helper within the area
    MobilityHelper CreateMobility(unsigned areaWidth, unsigned areaHeight);
};

int 
main(int argc, char *argv[])
{
  bool verbose = true;
  uint32_t nAmiNodes = 3;
  bool tracing = false;
  bool useMeshUnder = false;
  unsigned areaWidth = 50;
  unsigned areaHeight = 50;
  double durationSeconds = 10.0;

  CommandLine cmd(__FILE__);
  cmd.AddValue("nAmiNodes", "Number of AMI devices", nAmiNodes);
  cmd.AddValue("durationSeconds", "Number seconds to run the simulation", durationSeconds);
  cmd.AddValue("useMeshUnder", "Use mesh-under LR-WPAN routing if true", useMeshUnder);
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
  test.CaseRun(nAmiNodes, 
               areaWidth,
               areaHeight,
               durationSeconds,
               useMeshUnder,
               verbose,
               tracing);
}

void
AmiExample::CaseRun(uint32_t nAmiNodes,
                    unsigned areaWidth,
                    unsigned areaHeight,
                    double durationSeconds,
                    bool useMeshUnder,
                    bool verbose,
                    bool tracing)
{
  NodeContainer amiNodes = CreateNodes(nAmiNodes);

  LrWpanHelper lrWpanHelper;
  // Add and install the LrWpanNetDevice for each node
  NetDeviceContainer lrwpanDevices = lrWpanHelper.Install(amiNodes);

  // Fake PAN association and short address assignment.
  // This is needed because the lr-wpan module does not provide (yet)
  // a full PAN association procedure.
  lrWpanHelper.AssociateToPan(lrwpanDevices, 0);
  
  SixLowPanHelper sixLowPanHelper;
  NetDeviceContainer amiDevices = sixLowPanHelper.Install(lrwpanDevices);

  MobilityHelper mobility = CreateMobility(areaWidth, areaHeight);
  mobility.Install(amiNodes);

  Ipv6ListRoutingHelper listRh;
  Ipv6StaticRoutingHelper staticRh;
  listRh.Add(staticRh, 1);

  InternetStackHelper stack;
  stack.SetIpv4StackInstall(false); // IPv6 only
  stack.SetRoutingHelper(listRh);
  stack.Install(amiNodes);

  Ipv6AddressHelper address;
  address.SetBase(Ipv6Address("2001:1::"), Ipv6Prefix(64));
  Ipv6InterfaceContainer staInterfaces;
  staInterfaces = address.Assign(amiDevices);
  staInterfaces.SetForwarding(0, true);
  staInterfaces.SetDefaultRouteInAllNodes(0);
 
  if (useMeshUnder) 
  {
      for (uint32_t i = 0; i < amiDevices.GetN(); i++)
      {
          Ptr<NetDevice> dev = amiDevices.Get(i);
          dev->SetAttribute("UseMeshUnder", BooleanValue(true));
          dev->SetAttribute("MeshUnderRadius", UintegerValue(10));
      }
  }

  // install the echo server on the last node
  UdpEchoServerHelper echoServer(9);
  ApplicationContainer serverApps = echoServer.Install(amiNodes.Get(nAmiNodes - 1));
  serverApps.Start(Seconds(1.0));
  serverApps.Stop(Seconds(durationSeconds));

  // install the echo server on the first node
  UdpEchoClientHelper echoClient(staInterfaces.GetAddress(nAmiNodes - 1, 1), 9);
  echoClient.SetAttribute("MaxPackets", UintegerValue(1));
  echoClient.SetAttribute("Interval", TimeValue(Seconds(1.0)));
  echoClient.SetAttribute("PacketSize", UintegerValue(1024));
  ApplicationContainer clientApps = echoClient.Install(amiNodes.Get(0));

  clientApps.Start(Seconds(3.0));
  clientApps.Stop(Seconds(durationSeconds));

  Simulator::Stop(Seconds(durationSeconds));

  if(tracing)
  {
      lrWpanHelper.EnablePcap("ami", lrwpanDevices.Get(0));
      lrWpanHelper.EnablePcap("ami", lrwpanDevices.Get(nAmiNodes - 1));
      //lrWpanHelper.EnablePcapAll("ami", true);
  }

  Simulator::Run();
  Simulator::Destroy();
}

NodeContainer
AmiExample::CreateNodes(uint32_t nAmiNodes)
{
  NodeContainer amiNodes;
  amiNodes.Create(nAmiNodes);
  return amiNodes;
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
