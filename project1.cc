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

// Default network topology includes some number of AP nodes specified by
// the variable nWifis (defaults to two).  Off of each AP node, there are some
// number of STA nodes specified by the variable nStas (defaults to two).
// Each AP talks to its associated STA nodes.  There are bridge net devices
// on each AP node that bridge the whole thing into one network.
//
// Default Network Topology
//
//      *********************************** Topology ***********************************
//      +-----+      +-----+      +-----+         +-----+      +-----+      +-----+ 
//      | STA |      | STA |      | STA |         | STA |      | STA |      | STA |
//      +-----+      +-----+      +-----+         +-----+      +-----+      +-----+
//     IPaddress    IPaddress     IPaddress      IPaddress     IPaddress   IPaddress
// 
//       Node 2       Node 3       Node 4          Node 5       Node 6       Node 7
//      --------     --------     --------        --------     --------     --------
//      WIFI STA     WIFI STA     WIFI STA        WIFI STA     WIFI STA     WIFI STA
//      --------     --------     --------        --------     --------     --------
//        ((*))       ((*))         ((*))    |      ((*))        ((*))        ((*))
//                                           |
//                    ((*))                  |                   ((*))
//                   -------                                    -------
//                   WIFI AP         CSMA ========= CSMA        WIFI AP
//                   -------         ----           ----        -------
//                ##############                             ##############
//                    BRIDGE                                     BRIDGE
//                ##############                             ##############
//                   IPaddress                                  IPaddress
// 
//                    Node 0                                      Node 1
//                  +---------+                                +---------+
//                  | AP Node |                                | AP Node |
//                  +---------+                                +---------+
//      *********************************** Topology ***********************************

#include "ns3/core-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/network-module.h"
#include "ns3/applications-module.h"
#include "ns3/mobility-module.h"
#include "ns3/csma-module.h"
#include "ns3/internet-module.h"
#include "ns3/yans-wifi-helper.h"
#include "ns3/ssid.h"
#include "ns3/netanim-module.h"
#include <fstream>
#include "ns3/core-module.h"
#include "ns3/internet-module.h"
#include "ns3/csma-module.h"
#include "ns3/internet-apps-module.h"
#include "ns3/ipv6-header.h"
#include "ns3/packet-sink.h"
#include "ns3/packet-sink-helper.h"
#include "ns3/command-line.h"
#include "ns3/double.h"
#include "ns3/uinteger.h"
#include "ns3/rectangle.h"
#include "ns3/string.h"
#include "ns3/mobility-helper.h"
#include "ns3/internet-stack-helper.h"
#include "ns3/on-off-helper.h"
#include "ns3/yans-wifi-channel.h"
#include "ns3/csma-helper.h"
#include "ns3/bridge-helper.h"
#include "ns3/packet-socket-address.h"


using namespace ns3;

int main (int argc, char *argv[])
{
  uint32_t nWifis = 2;
  uint32_t nStas = 3;
  bool sendIp = true;
  bool writeMobility = false;

  CommandLine cmd (__FILE__);
  cmd.AddValue ("nWifis", "Number of wifi networks", nWifis);
  cmd.AddValue ("nStas", "Number of stations per wifi network", nStas);
  cmd.AddValue ("SendIp", "Send Ipv4 or raw packets", sendIp);
  cmd.AddValue ("writeMobility", "Write mobility trace", writeMobility);
  cmd.Parse (argc, argv);

  NodeContainer backboneNodes;
  NetDeviceContainer backboneDevices;
  Ipv6InterfaceContainer backboneInterfaces;
  std::vector<NodeContainer> staNodes;
  std::vector<NetDeviceContainer> staDevices;
  std::vector<NetDeviceContainer> apDevices;
  std::vector<Ipv6InterfaceContainer> staInterfaces;
  std::vector<Ipv6InterfaceContainer> apInterfaces;

  InternetStackHelper stack;
  CsmaHelper csma;
  Ipv6AddressHelper ip;
  ip.SetBase (Ipv6Address ("2001:1::"), Ipv6Prefix (64));

  backboneNodes.Create (nWifis);
  stack.Install (backboneNodes);

  backboneDevices = csma.Install (backboneNodes);

  double wifiX = 0.0;

  YansWifiPhyHelper wifiPhy;
  wifiPhy.SetPcapDataLinkType (WifiPhyHelper::DLT_IEEE802_11_RADIO);

  for (uint32_t i = 0; i < nWifis; ++i)
    {
      // calculate ssid for wifi subnetwork
      std::ostringstream oss;
      oss << "wifi-default-" << i;
      Ssid ssid = Ssid (oss.str ());

      NodeContainer sta;
      NetDeviceContainer staDev;
      NetDeviceContainer apDev;
      Ipv6InterfaceContainer staInterface;
      Ipv6InterfaceContainer apInterface;
      MobilityHelper mobility;
      BridgeHelper bridge;
      WifiHelper wifi;
      WifiMacHelper wifiMac;
      YansWifiChannelHelper wifiChannel = YansWifiChannelHelper::Default ();
      // channel1 & channel2
      wifiPhy.SetChannel (wifiChannel.Create ());

      sta.Create (nStas);
      mobility.SetPositionAllocator ("ns3::GridPositionAllocator",
                                     "MinX", DoubleValue (wifiX),
                                     "MinY", DoubleValue (0.0),
                                     "DeltaX", DoubleValue (5.0),
                                     "DeltaY", DoubleValue (5.0),
                                     "GridWidth", UintegerValue (1),
                                     "LayoutType", StringValue ("RowFirst"));

      // setup the AP.
      mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
      mobility.Install (backboneNodes.Get (i));
      wifiMac.SetType ("ns3::ApWifiMac",
                       "Ssid", SsidValue (ssid));
      apDev = wifi.Install (wifiPhy, wifiMac, backboneNodes.Get (i));

      NetDeviceContainer bridgeDev;
      bridgeDev = bridge.Install (backboneNodes.Get (i), NetDeviceContainer (apDev, backboneDevices.Get (i)));

      // assign AP IP address to bridge, not wifi
      apInterface = ip.Assign (bridgeDev);

      // setup the STAs
      stack.Install (sta);
      mobility.SetMobilityModel ("ns3::RandomWalk2dMobilityModel",
                                 "Mode", StringValue ("Time"),
                                 "Time", StringValue ("2s"),
                                 "Speed", StringValue ("ns3::ConstantRandomVariable[Constant=1.0]"),
                                 "Bounds", RectangleValue (Rectangle (wifiX, wifiX + 5.0,0.0, (nStas + 1) * 5.0)));
      mobility.Install (sta);
      wifiMac.SetType ("ns3::StaWifiMac",
                       "Ssid", SsidValue (ssid));
      staDev = wifi.Install (wifiPhy, wifiMac, sta);
      staInterface = ip.Assign (staDev);

      // save everything in containers.
      staNodes.push_back (sta);
      apDevices.push_back (apDev);
      apInterfaces.push_back (apInterface);
      staDevices.push_back (staDev);
      staInterfaces.push_back (staInterface);

      wifiX += 20.0;
    }

  Address dest;
  std::string protocol;
  if (sendIp)
    {
      dest = Inet6SocketAddress (staInterfaces[1].GetAddress (1,1), 1025);
      protocol = "ns3::UdpSocketFactory";
    }
  else
    {
      PacketSocketAddress tmp;
      tmp.SetSingleDevice (staDevices[0].Get (0)->GetIfIndex ());
      tmp.SetPhysicalAddress (staDevices[1].Get (0)->GetAddress ());
      tmp.SetProtocol (0x807);
      dest = tmp;
      protocol = "ns3::PacketSocketFactory";
    }

  // 此處為架設 Server UDP Application 與 Client UDP Application
  // 此處只架設了一個 Server UDP Application 與一個 Client UDP Application 
  // Server 端為接收端，Client 端為發送端，Server 端接收封包後會回傳封包給 Client 端
  // 在下列範例 code 中 Server 端架設在第一個 Wifi 下的 STA Node 0，Client 端架設在第二個 Wifi 下的 STA Node 0
  // StaNodes[0] 中儲存了第一個 Wifi 下的所有 STA Node，StaNodes[1] 中儲存了第二個 Wifi 下的所有 STA Node
  // 同學可以自行更改，利用 for 迴圈進行增加，架設 3 對一對一的 Server 與 Client UDP Application，每對 Server 與 Client Application 都要有自己所使用的 port
  //***********************************************//
  uint16_t port = 1000;
  uint32_t packetSize = 1024;
  uint32_t maxPacketCount = 10;
  // 此處可以設定 Application 傳送封包的間隔時間
  Time interPacketInterval = Seconds (1.);

  uint32_t i = 0;

  UdpEchoServerHelper server(port);

  ApplicationContainer serverApp0 = server.Install(staNodes[0].Get(0));
  ApplicationContainer serverApp1 = server.Install(staNodes[0].Get(1));
  ApplicationContainer serverApp2 = server.Install(staNodes[0].Get(2));

  UdpEchoClientHelper client(staInterfaces[０].GetAddress(i, 0), port);
  client.SetAttribute ("MaxPackets", UintegerValue (maxPacketCount));
  client.SetAttribute ("Interval", TimeValue (interPacketInterval));
  client.SetAttribute ("PacketSize", UintegerValue (packetSize));

  ApplicationContainer clientApp0 = client0.Install(staNodes[1].Get(0));
  ApplicationContainer clientApp1 = client1.Install(staNodes[1].Get(1));
  ApplicationContainer clientApp2 = client2.Install(staNodes[1].Get(2));

  
  // set start and stop time of the server application
  serverApp0.Start(Seconds(1.0));
  serverApp1.Start(Seconds(1.0));
  serverApp2.Start(Seconds(1.0));
  serverApp0.Stop(Seconds(10.0));
  serverApp1.Stop(Seconds(10.0));  
  serverApp2.Stop(Seconds(10.0));
  
  // set start and stop time of the client application
  clientApp0.Start(Seconds(2.0));
  clientApp1.Start(Seconds(2.0));
  clientApp2.Start(Seconds(2.0));
  clientApp0.Stop(Seconds(10.0));
  clientApp1.Stop(Seconds(10.0));  
  clientApp2.Stop(Seconds(10.0));
  //***********************************************//

  // 此處為擷取 csma 封包
  // 執行完程式後會產生兩個 pcap 檔 (p1-0-1.pcap, p1-1-1.pcap)
  // 同學再依據 pcap 檔內擷取的封包進行觀察撰寫報告即可
  // 例如：ipv6協定傳送了什麼封包，這些封包的目的是什麼...等
  csma.EnablePcap("p1", backboneDevices.Get(0));
  csma.EnablePcap("p1", backboneDevices.Get(1));



  // OnOffHelper onoff = OnOffHelper (protocol, dest);
  // onoff.SetConstantRate (DataRate ("500kb/s"));
  // ApplicationContainer apps = onoff.Install (staNodes[0].Get (0));
  // apps.Start (Seconds (0.5));
  // apps.Stop (Seconds (3.0));

  // // add UDP client & Server
  // UdpEchoServerHelper echoServer (9);

  // ApplicationContainer serverApps1 = echoServer.Install (staNodes[0].Get (0)); // Server 架在wlan1 的node上
  // ApplicationContainer serverApps2 = echoServer.Install (staNodes[0].Get (1));
  // ApplicationContainer serverApps3 = echoServer.Install (staNodes[0].Get (2));
  // serverApps1.Start (Seconds (1.0));
  // serverApps2.Start (Seconds (1.0));
  // serverApps3.Start (Seconds (1.0));
  // serverApps1.Stop (Seconds (10.0));
  // serverApps2.Stop (Seconds (10.0));
  // serverApps3.Stop (Seconds (10.0));

  // UdpEchoClientHelper echoClient (staInterfaces[1].GetAddress (1,1), 9); 
  // echoClient.SetAttribute ("MaxPackets", UintegerValue (1));
  // echoClient.SetAttribute ("Interval", TimeValue (Seconds (1.0)));
  // echoClient.SetAttribute ("PacketSize", UintegerValue (1024));

  // ApplicationContainer clientApps1 = echoClient.Install (staNodes[1].Get (0));  // Client在wlan2 的node
  // ApplicationContainer clientApps2 = echoClient.Install (staNodes[1].Get (1));
  // ApplicationContainer clientApps3 = echoClient.Install (staNodes[1].Get (2));
  // clientApps1.Start (Seconds (2.0));
  // clientApps2.Start (Seconds (2.0));
  // clientApps3.Start (Seconds (2.0));
  // clientApps1.Stop (Seconds (10.0));
  // clientApps2.Stop (Seconds (10.0));
  // clientApps3.Stop (Seconds (10.0));

  // wifiPhy.EnablePcap ("proj1-new2", apDevices[0]);
  // wifiPhy.EnablePcap ("proj1-new2", apDevices[1]);
  // wifiPhy.EnablePcap ("proj1-new2", staDevices[0].Get(0));
  // wifiPhy.EnablePcap ("proj1-new2", staDevices[1].Get(0));

  // if (writeMobility)
  //   {
  //     AsciiTraceHelper ascii;
  //     MobilityHelper::EnableAsciiAll (ascii.CreateFileStream ("wifi-wired-bridging.mob"));
  //   }

  Simulator::Stop (Seconds (5.0));
  AnimationInterface anim ("myproj1-new2.xml");
  anim.SetStartTime (Seconds(0));
  anim.SetStopTime (Seconds(10));

  Simulator::Run ();
  Simulator::Destroy ();
}
