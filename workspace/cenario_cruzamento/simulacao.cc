/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * ==============================================================================
 * simulacao.cc - Comunicação Veicular V2I (SUMO + NS-3 em 2D)
 * ==============================================================================
 * Inspirado nos exemplos canônicos do NS-3 e nos tutoriais de Adil Alsuhaim:
 *   - Nó 0: Carro 1 (vindo do Oeste em direção ao Leste)
 *   - Nó 1: Carro 2 (vindo do Sul em direção ao Norte)
 *   - Nó 2: Antena RSU na esquina Nordeste (112.0, 112.0, 0.0)
 *   - Nó 3: Prédio no quadrante Sudoeste (Barreira visual 2D)
 *
 * Utiliza aplicações nativas UdpEchoServer e UdpEchoClient para máxima
 * simplicidade, clareza e padronização.
 * ==============================================================================
 */

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/mobility-module.h"
#include "ns3/ns2-mobility-helper.h"
#include "ns3/wifi-module.h"
#include "ns3/internet-module.h"
#include "ns3/applications-module.h"
#include "ns3/netanim-module.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("SimulacaoCruzamentoSimples");

int main (int argc, char *argv[])
{
  CommandLine cmd;
  cmd.Parse (argc, argv);

  // Ativa os logs informativos nativos do UdpEcho (padrão dos tutoriais do NS-3)
  LogComponentEnable ("UdpEchoClientApplication", LOG_LEVEL_INFO);
  LogComponentEnable ("UdpEchoServerApplication", LOG_LEVEL_INFO);


  // ============================================================================
  // ETAPA 2: CRIAÇÃO DOS NÓS DA REDE (CARROS E ANTENA)
  // ============================================================================
  // Criamos os dois carros autônomos
  NodeContainer carNodes;
  carNodes.Create (2); // Nó 0 = Carro 1 | Nó 1 = Carro 2

  // Criamos a Antena de Infraestrutura (RSU)
  NodeContainer rsuNode;
  rsuNode.Create (1);  // Nó 2 = Antena RSU

  // Criamos o nó para representação visual do prédio no NetAnim
  NodeContainer buildingNode;
  buildingNode.Create (1); // Nó 3 = Prédio (Barreira NLoS)

  // Agrupamos os nós comunicantes (Carros + Antena)
  NodeContainer wifiNodes;
  wifiNodes.Add (carNodes);
  wifiNodes.Add (rsuNode);


  // ============================================================================
  // ETAPA 3: CONFIGURAÇÃO DA MOBILIDADE EM 2D (SUMO E POSIÇÃO FIXA)
  // ============================================================================
  // 3.1 Mobilidade 2D dos Carros via traço do SUMO
  std::string traceFile = "/workspace/cenario_cruzamento/mobilidade.tcl";
  Ns2MobilityHelper sumoTrace (traceFile);
  sumoTrace.Install (carNodes.Begin (), carNodes.End ());

  // 3.2 Posição Fixa 2D da Antena (RSU): Esquina Nordeste (112.0, 112.0, 0.0)
  MobilityHelper rsuMobility;
  Ptr<ListPositionAllocator> rsuPos = CreateObject<ListPositionAllocator> ();
  rsuPos->Add (Vector (112.0, 112.0, 0.0));
  rsuMobility.SetPositionAllocator (rsuPos);
  rsuMobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  rsuMobility.Install (rsuNode);

  // 3.3 Posição Fixa 2D do Prédio: Centro da barreira Sudoeste (64.0, 64.0, 0.0)
  MobilityHelper bldgMobility;
  Ptr<ListPositionAllocator> bldgPos = CreateObject<ListPositionAllocator> ();
  bldgPos->Add (Vector (64.0, 64.0, 0.0));
  bldgMobility.SetPositionAllocator (bldgPos);
  bldgMobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  bldgMobility.Install (buildingNode);


  // ============================================================================
  // ETAPA 4: CONFIGURAÇÃO DA REDE SEM FIO (WI-FI AD-HOC) E IP
  // ============================================================================
  // Configuração do canal de rádio sem fio
  YansWifiChannelHelper wifiChannel = YansWifiChannelHelper::Default ();
  YansWifiPhyHelper wifiPhy = YansWifiPhyHelper::Default ();
  wifiPhy.SetChannel (wifiChannel.Create ());

  // Placa de rede Wi-Fi em modo Ad-hoc (comunicação veicular direta)
  NqosWifiMacHelper wifiMac = NqosWifiMacHelper::Default ();
  wifiMac.SetType ("ns3::AdhocWifiMac");

  WifiHelper wifi;
  wifi.SetStandard (WIFI_PHY_STANDARD_80211a);
  NetDeviceContainer wifiDevices = wifi.Install (wifiPhy, wifiMac, wifiNodes);

  // Instalação da pilha TCP/IP e endereçamento
  InternetStackHelper stack;
  stack.Install (wifiNodes);

  Ipv4AddressHelper address;
  address.SetBase ("10.1.1.0", "255.255.255.0");
  Ipv4InterfaceContainer interfaces = address.Assign (wifiDevices);


  // ============================================================================
  // ETAPA 5: APLICAÇÕES DE COMUNICAÇÃO VIA INFRAESTRUTURA E NETANIM
  // ============================================================================
  // 5.1 Servidor na Antena (Porta 9): responde às mensagens dos veículos
  UdpEchoServerHelper echoServer (9);
  ApplicationContainer serverApp = echoServer.Install (rsuNode.Get (0));
  serverApp.Start (Seconds (1.0));
  serverApp.Stop (Seconds (25.0));

  // 5.2 Cliente no Carro 1: envia mensagens de alerta para a Antena (10.1.1.3)
  UdpEchoClientHelper client1 (interfaces.GetAddress (2), 9);
  client1.SetAttribute ("MaxPackets", UintegerValue (20));
  client1.SetAttribute ("Interval", TimeValue (Seconds (1.0)));
  client1.SetAttribute ("PacketSize", UintegerValue (512));

  ApplicationContainer clientApp1 = client1.Install (carNodes.Get (0));
  clientApp1.Start (Seconds (2.0));
  clientApp1.Stop (Seconds (25.0));

  // 5.3 Cliente no Carro 2: também se comunica com a Antena (10.1.1.3)
  UdpEchoClientHelper client2 (interfaces.GetAddress (2), 9);
  client2.SetAttribute ("MaxPackets", UintegerValue (20));
  client2.SetAttribute ("Interval", TimeValue (Seconds (1.0)));
  client2.SetAttribute ("PacketSize", UintegerValue (512));

  ApplicationContainer clientApp2 = client2.Install (carNodes.Get (1));
  clientApp2.Start (Seconds (2.5));
  clientApp2.Stop (Seconds (25.0));

  // 5.4 Visualização Gráfica no NetAnim (2D)
  AnimationInterface anim ("cruzamento-animacao.xml");
  anim.EnablePacketMetadata (true);

  // Cores e tamanhos personalizados
  anim.UpdateNodeDescription (carNodes.Get (0), "Carro 1 (Oeste)");
  anim.UpdateNodeColor (carNodes.Get (0), 0, 120, 255); // Azul
  anim.UpdateNodeSize (carNodes.Get (0)->GetId (), 4.0, 4.0);

  anim.UpdateNodeDescription (carNodes.Get (1), "Carro 2 (Sul)");
  anim.UpdateNodeColor (carNodes.Get (1), 255, 140, 0); // Laranja
  anim.UpdateNodeSize (carNodes.Get (1)->GetId (), 4.0, 4.0);

  anim.UpdateNodeDescription (rsuNode.Get (0), "Antena RSU");
  anim.UpdateNodeColor (rsuNode.Get (0), 255, 215, 0); // Dourado
  anim.UpdateNodeSize (rsuNode.Get (0)->GetId (), 6.0, 6.0);

  anim.UpdateNodeDescription (buildingNode.Get (0), "Predio (NLoS)");
  anim.UpdateNodeColor (buildingNode.Get (0), 80, 80, 80); // Cinza
  anim.UpdateNodeSize (buildingNode.Get (0)->GetId (), 48.0, 48.0); // 48x48 metros

  // Execução da Simulação
  Simulator::Stop (Seconds (25.0));
  Simulator::Run ();
  Simulator::Destroy ();

  return 0;
}
