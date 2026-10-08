/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * ==============================================================================
 * simulacao.cc - Comunicação Veicular V2V e V2I/RIS (SUMO + NS-3 em 2D)
 * ==============================================================================
 * Cenário de Cruzamento Urbano com Bloqueio NLoS e Antena RIS:
 *   - Nó 0: Carro 1 (TX - aproximação Oeste em direção ao Leste, rota eastbound)
 *   - Nó 1: Carro 2 (RX - aproximação Sul em direção ao Norte, rota northbound)
 *   - Nó 2: Antena / Metasuperfície RIS na esquina Nordeste (112.0, 112.0, 0.0)
 *   - Nó 3: Prédio no quadrante Sudoeste (Barreira NLoS [40, 88] x [40, 88])
 *
 * Dinâmica de Comunicação:
 *   1. Fase NLoS e Fora de Alcance: Carros distantes, sinal direto bloqueado
 *      pelo edifício e fora do alcance da RIS -> Carro anda sem sinal (desconectado).
 *   2. Fase NLoS Assistida por RIS: Carros entram na cobertura útil da RIS
 *      (esquina Nordeste). A RIS começa a funcionar e contorna a barreira física
 *      -> Comunicação de Carro para Antena (via RIS).
 *   3. Fase LoS Direto: Os veículos ultrapassam a quina do prédio e entram
 *      no cruzamento com linha de visada livre -> Comunicação direta Carro para Carro (V2V).
 *
 * Métricas Coletadas e Exportadas para Arquivo TXT:
 *   1. Quantidade de mensagens diretas Carro-a-Carro vs. Carro-a-Antena (RIS).
 *   2. Tempo em que o carro anda sem sinal vs. tempo se comunicando.
 *   3. Instante exato em que a RIS começa a funcionar.
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

#include <iostream>
#include <fstream>
#include <vector>
#include <iomanip>
#include <cmath>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("SimulacaoCruzamentoRIS");

// ==============================================================================
// ESTRUTURAS E VARIÁVEIS GLOBAIS DE COLETA DE MÉTRICAS
// ==============================================================================
struct LogRegistro {
  double tempo;
  double c1_x, c1_y;
  double c2_x, c2_y;
  double distV2V;
  double distC1Ris;
  double distC2Ris;
  bool bloqueadoEdificio;
  std::string modo; // "SEM_SINAL", "VIA_ANTENA_RIS", "V2V_DIRETO"
};

static uint32_t g_msgDiretoCarroParaCarro = 0;
static uint32_t g_msgCarroParaAntena = 0;
static uint32_t g_tentativasSemSinal = 0;
static double   g_tempoSemSinal = 0.0;
static double   g_tempoComunicando = 0.0;
static double   g_instanteInicioRis = -1.0;
static std::vector<LogRegistro> g_historicoLogs;

// ==============================================================================
// ALGORITMO GEOMÉTRICO: INTERSEÇÃO ENTRE SEGMENTO DE RETA E RETÂNGULO 2D
// (Baseado em Liang-Barsky para detecção exata de bloqueio NLoS por obstáculos)
// ==============================================================================
bool LineIntersectsBox (double x1, double y1, double x2, double y2,
                        double xmin, double ymin, double xmax, double ymax)
{
  double dx = x2 - x1;
  double dy = y2 - y1;
  double p[4] = {-dx, dx, -dy, dy};
  double q[4] = {x1 - xmin, xmax - x1, y1 - ymin, ymax - y1};

  double u1 = 0.0;
  double u2 = 1.0;

  for (int i = 0; i < 4; ++i)
    {
      if (p[i] == 0.0)
        {
          if (q[i] < 0.0)
            {
              return false; // Reta paralela e fora do retângulo
            }
        }
      else
        {
          double t = q[i] / p[i];
          if (p[i] < 0.0)
            {
              if (t > u2) return false;
              if (t > u1) u1 = t;
            }
          else
            {
              if (t < u1) return false;
              if (t < u2) u2 = t;
            }
        }
    }
  return (u1 <= u2);
}

// ==============================================================================
// CALLBACKS DOS SOCKETS REPOSITÓRIOS / RECEPTORES
// ==============================================================================
void RsuReceivePacket (Ptr<Socket> socket)
{
  Ptr<Packet> packet;
  Address from;
  while ((packet = socket->RecvFrom (from)))
    {
      // Cria um novo pacote de dados limpo para retransmissão / reflexão sem conflito de tags
      Ptr<Packet> fwdPacket = Create<Packet> (packet->GetSize ());
      InetSocketAddress dest (Ipv4Address ("10.1.1.2"), 9);
      socket->SendTo (fwdPacket, 0, dest);
    }
}

void Car2ReceivePacket (Ptr<Socket> socket)
{
  Ptr<Packet> packet;
  Address from;
  while ((packet = socket->RecvFrom (from)))
    {
      // Pacote de alerta veicular recebido com sucesso pelo Carro 2
    }
}

// ==============================================================================
// CONTEXTO DE SIMULAÇÃO PARA AGENDAMENTO DE EVENTOS
// ==============================================================================
struct SimContext {
  Ptr<Node> car1;
  Ptr<Node> car2;
  Ptr<Node> rsu;
  Ptr<Socket> txSocket;
  double interval;
  double risRange;
};

// ==============================================================================
// TRANSMISSÃO PERIÓDICA DE MENSAGENS VEICULARES E MONITORAMENTO
// ==============================================================================
void PeriodicTransmission (SimContext ctx)
{
  double now = Simulator::Now ().GetSeconds ();

  Ptr<MobilityModel> mob1 = ctx.car1->GetObject<MobilityModel> ();
  Ptr<MobilityModel> mob2 = ctx.car2->GetObject<MobilityModel> ();
  Ptr<MobilityModel> mobRsu = ctx.rsu->GetObject<MobilityModel> ();

  Vector p1 = mob1->GetPosition ();
  Vector p2 = mob2->GetPosition ();
  Vector pRsu = mobRsu->GetPosition ();

  double distV2V = mob1->GetDistanceFrom (mob2);
  double distC1Ris = mob1->GetDistanceFrom (mobRsu);
  double distC2Ris = mob2->GetDistanceFrom (mobRsu);

  // Prédio Sudoeste: X in [40.0, 88.0], Y in [40.0, 88.0]
  bool blocked = LineIntersectsBox (p1.x, p1.y, p2.x, p2.y, 40.0, 40.0, 88.0, 88.0);
  bool risCoverage = (distC1Ris <= ctx.risRange) && (distC2Ris <= ctx.risRange);

  LogRegistro reg;
  reg.tempo = now;
  reg.c1_x = p1.x; reg.c1_y = p1.y;
  reg.c2_x = p2.x; reg.c2_y = p2.y;
  reg.distV2V = distV2V;
  reg.distC1Ris = distC1Ris;
  reg.distC2Ris = distC2Ris;
  reg.bloqueadoEdificio = blocked;

  if (!blocked)
    {
      // Caso 1: Linha de Visada Direta Livre (LoS) -> Mensagem DIRETO CARRO PARA CARRO
      Ptr<Packet> pkt = Create<Packet> (512);
      InetSocketAddress dest (Ipv4Address ("10.1.1.2"), 9);
      ctx.txSocket->SendTo (pkt, 0, dest);

      g_msgDiretoCarroParaCarro++;
      g_tempoComunicando += ctx.interval;
      reg.modo = "V2V_DIRETO";
    }
  else if (risCoverage)
    {
      // Caso 2: Bloqueio NLoS, mas na área de cobertura da RIS -> MENSAGEM VIA ANTENA / RIS
      if (g_instanteInicioRis < 0.0)
        {
          g_instanteInicioRis = now;
        }

      Ptr<Packet> pkt = Create<Packet> (512);
      InetSocketAddress dest (Ipv4Address ("10.1.1.3"), 9);
      ctx.txSocket->SendTo (pkt, 0, dest);

      g_msgCarroParaAntena++;
      g_tempoComunicando += ctx.interval;
      reg.modo = "VIA_ANTENA_RIS";
    }
  else
    {
      // Caso 3: Bloqueio NLoS e fora do alcance da RIS -> VEÍCULO SEM SINAL
      g_tentativasSemSinal++;
      g_tempoSemSinal += ctx.interval;
      reg.modo = "SEM_SINAL";
    }

  g_historicoLogs.push_back (reg);

  // Reagenda até os carros completarem a travessia (15.5s)
  if (now + ctx.interval <= 15.5)
    {
      Simulator::Schedule (Seconds (ctx.interval), &PeriodicTransmission, ctx);
    }
}

// ==============================================================================
// GERAÇÃO DO ARQUIVO TXT COM OS RESULTADOS DETALHADOS
// ==============================================================================
void ExportarRelatorioTxt (std::string filename, double tempoTotalViagem,
                           double interval, double risRange)
{
  std::ofstream out (filename.c_str ());
  if (!out.is_open ())
    {
      std::cerr << "Erro ao abrir o arquivo para escrita: " << filename << std::endl;
      return;
    }

  uint32_t totalMensagens = g_msgDiretoCarroParaCarro + g_msgCarroParaAntena + g_tentativasSemSinal;
  double pctComunicando = (tempoTotalViagem > 0.0) ? (g_tempoComunicando / tempoTotalViagem) * 100.0 : 0.0;
  double pctSemSinal    = (tempoTotalViagem > 0.0) ? (g_tempoSemSinal / tempoTotalViagem) * 100.0 : 0.0;

  out << "===============================================================================\n";
  out << "      RELATÓRIO DE COMUNICAÇÃO VEICULAR NO CENÁRIO DE CRUZAMENTO (NS-3 / SUMO) \n";
  out << "===============================================================================\n\n";

  out << "1. VERIFICAÇÃO DE MENSAGENS (DIRETO CARRO-A-CARRO vs. CARRO-A-ANTENA):\n";
  out << "-------------------------------------------------------------------------------\n";
  out << "  • Mensagens DIRETO de Carro para Carro (V2V em LoS): " << g_msgDiretoCarroParaCarro << "\n";
  out << "  • Mensagens de Carro para Antena (V2I assistido por RIS em NLoS): " << g_msgCarroParaAntena << "\n";
  out << "  • Tentativas em que o veículo esteve Sem Sinal (NLoS fora da RIS): " << g_tentativasSemSinal << "\n";
  out << "  • Total de ciclos de transmissão: " << totalMensagens << "\n\n";

  out << "2. ANÁLISE TEMPORAL (TEMPO SEM SINAL vs. TEMPO SE COMUNICANDO):\n";
  out << "-------------------------------------------------------------------------------\n";
  out << "  • Duração total do deslocamento veicular: " << std::fixed << std::setprecision (2) << tempoTotalViagem << " s\n";
  out << "  • Tempo em que o carro anda SEM SINAL (desconectado): " << g_tempoSemSinal << " s (" << pctSemSinal << "% do tempo)\n";
  out << "  • Tempo em que o carro passa SE COMUNICANDO (ativo): " << g_tempoComunicando << " s (" << pctComunicando << "% do tempo)\n\n";

  out << "3. INSTANTE DE ATIVAÇÃO DA ANTENA RIS:\n";
  out << "-------------------------------------------------------------------------------\n";
  if (g_instanteInicioRis >= 0.0)
    {
      out << "  • Instante em que o RIS começa a funcionar: t = " << g_instanteInicioRis << " s\n";
      out << "  • Descrição: A partir deste instante, os dois veículos entram no raio de\n";
      out << "    cobertura útil da RIS (" << risRange << " m) e a RIS redireciona os sinais\n";
      out << "    contornando o prédio até que os veículos alcancem linha de visada direta.\n\n";
    }
  else
    {
      out << "  • O RIS não foi ativado durante a simulação.\n\n";
    }

  out << "===============================================================================\n";
  out << "4. HISTÓRICO SEGUNDO A SEGUNDO DA SIMULAÇÃO:\n";
  out << "===============================================================================\n";
  out << std::left
      << std::setw (10) << "Tempo(s)"
      << std::setw (18) << "Pos Carro 1"
      << std::setw (18) << "Pos Carro 2"
      << std::setw (14) << "Dist V2V(m)"
      << std::setw (14) << "Dist RIS(m)"
      << std::setw (16) << "Visada V2V"
      << std::setw (16) << "Estado / Modo"
      << "\n";
  out << "------------------------------------------------------------------------------------------------\n";

  for (size_t i = 0; i < g_historicoLogs.size (); ++i)
    {
      const LogRegistro &r = g_historicoLogs[i];
      std::ostringstream pos1, pos2;
      pos1 << "(" << std::fixed << std::setprecision (1) << r.c1_x << "," << r.c1_y << ")";
      pos2 << "(" << std::fixed << std::setprecision (1) << r.c2_x << "," << r.c2_y << ")";

      double maxDistRis = std::max (r.distC1Ris, r.distC2Ris);

      out << std::left
          << std::setw (10) << std::fixed << std::setprecision (1) << r.tempo
          << std::setw (18) << pos1.str ()
          << std::setw (18) << pos2.str ()
          << std::setw (14) << std::fixed << std::setprecision (1) << r.distV2V
          << std::setw (14) << std::fixed << std::setprecision (1) << maxDistRis
          << std::setw (16) << (r.bloqueadoEdificio ? "NLoS (Prédio)" : "LoS (Livre)")
          << std::setw (16) << r.modo
          << "\n";
    }

  out << "================================================================================================\n";
  out.close ();
}

// ==============================================================================
// FUNÇÃO PRINCIPAL (MAIN)
// ==============================================================================
int main (int argc, char *argv[])
{
  double beaconInterval = 1.0;  // Intervalo de transmissão (segundos)
  double risRange = 75.0;       // Alcance operacional útil da RIS (metros)
  double stopTime = 20.0;       // Duração da simulação no NS-3 (segundos)
  std::string outputTxt = "/workspace/cenario_cruzamento/resultado_comunicacao.txt";

  CommandLine cmd;
  cmd.AddValue ("interval", "Intervalo entre mensagens (s)", beaconInterval);
  cmd.AddValue ("risRange", "Raio de alcance da RIS (m)", risRange);
  cmd.AddValue ("stopTime", "Tempo final de simulação (s)", stopTime);
  cmd.AddValue ("outputTxt", "Caminho do arquivo txt de saída", outputTxt);
  cmd.Parse (argc, argv);

  // ============================================================================
  // ETAPA 2: CRIAÇÃO DOS NÓS DA REDE (CARROS, ANTENA E PRÉDIO)
  // ============================================================================
  NodeContainer carNodes;
  carNodes.Create (2); // Nó 0 = Carro 1 | Nó 1 = Carro 2

  NodeContainer rsuNode;
  rsuNode.Create (1);  // Nó 2 = Antena RSU / RIS na esquina

  NodeContainer buildingNode;
  buildingNode.Create (1); // Nó 3 = Prédio no quadrante Sudoeste

  NodeContainer wifiNodes;
  wifiNodes.Add (carNodes);
  wifiNodes.Add (rsuNode);

  // ============================================================================
  // ETAPA 3: CONFIGURAÇÃO DA MOBILIDADE (SUMO E POSIÇÕES FIXAS)
  // ============================================================================
  // 3.1 Mobilidade 2D dos Carros via traço do SUMO
  std::string traceFile = "/workspace/cenario_cruzamento/mobilidade.tcl";
  Ns2MobilityHelper sumoTrace (traceFile);
  sumoTrace.Install (carNodes.Begin (), carNodes.End ());

  // 3.2 Posição Fixa 2D da Antena (RSU / RIS): Esquina Nordeste (112.0, 112.0, 0.0)
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
  YansWifiChannelHelper wifiChannel = YansWifiChannelHelper::Default ();
  YansWifiPhyHelper wifiPhy = YansWifiPhyHelper::Default ();
  wifiPhy.SetChannel (wifiChannel.Create ());

  NqosWifiMacHelper wifiMac = NqosWifiMacHelper::Default ();
  wifiMac.SetType ("ns3::AdhocWifiMac");

  WifiHelper wifi;
  wifi.SetStandard (WIFI_PHY_STANDARD_80211a);
  NetDeviceContainer wifiDevices = wifi.Install (wifiPhy, wifiMac, wifiNodes);

  InternetStackHelper stack;
  stack.Install (wifiNodes);

  Ipv4AddressHelper address;
  address.SetBase ("10.1.1.0", "255.255.255.0");
  Ipv4InterfaceContainer interfaces = address.Assign (wifiDevices);

  // ============================================================================
  // ETAPA 5: SOCKETS UDP E COMUNICAÇÃO VEICULAR
  // ============================================================================
  // Receptor no Carro 2 (Porta 9)
  Ptr<Socket> rxSocketCar2 = Socket::CreateSocket (carNodes.Get (1), TypeId::LookupByName ("ns3::UdpSocketFactory"));
  InetSocketAddress localCar2 (Ipv4Address::GetAny (), 9);
  rxSocketCar2->Bind (localCar2);
  rxSocketCar2->SetRecvCallback (MakeCallback (&Car2ReceivePacket));

  // Receptor na Antena RSU / RIS (Porta 9)
  Ptr<Socket> rxSocketRsu = Socket::CreateSocket (rsuNode.Get (0), TypeId::LookupByName ("ns3::UdpSocketFactory"));
  InetSocketAddress localRsu (Ipv4Address::GetAny (), 9);
  rxSocketRsu->Bind (localRsu);
  rxSocketRsu->SetRecvCallback (MakeCallback (&RsuReceivePacket));

  // Transmissor no Carro 1
  Ptr<Socket> txSocketCar1 = Socket::CreateSocket (carNodes.Get (0), TypeId::LookupByName ("ns3::UdpSocketFactory"));

  // Inicia transmissões periódicas a partir de t = 1.0s
  SimContext ctx;
  ctx.car1 = carNodes.Get (0);
  ctx.car2 = carNodes.Get (1);
  ctx.rsu = rsuNode.Get (0);
  ctx.txSocket = txSocketCar1;
  ctx.interval = beaconInterval;
  ctx.risRange = risRange;

  Simulator::Schedule (Seconds (1.0), &PeriodicTransmission, ctx);

  // ============================================================================
  // ETAPA 6: VISUALIZAÇÃO GRÁFICA NO NETANIM (2D)
  // ============================================================================
  AnimationInterface anim ("cruzamento-animacao.xml");
  anim.EnablePacketMetadata (true);

  anim.UpdateNodeDescription (carNodes.Get (0), "Carro 1 (Oeste/TX)");
  anim.UpdateNodeColor (carNodes.Get (0), 0, 120, 255); // Azul
  anim.UpdateNodeSize (carNodes.Get (0)->GetId (), 4.0, 4.0);

  anim.UpdateNodeDescription (carNodes.Get (1), "Carro 2 (Sul/RX)");
  anim.UpdateNodeColor (carNodes.Get (1), 255, 140, 0); // Laranja
  anim.UpdateNodeSize (carNodes.Get (1)->GetId (), 4.0, 4.0);

  anim.UpdateNodeDescription (rsuNode.Get (0), "Antena / RIS (Nordeste)");
  anim.UpdateNodeColor (rsuNode.Get (0), 255, 215, 0); // Dourado
  anim.UpdateNodeSize (rsuNode.Get (0)->GetId (), 6.0, 6.0);

  anim.UpdateNodeDescription (buildingNode.Get (0), "Predio (Barreira NLoS)");
  anim.UpdateNodeColor (buildingNode.Get (0), 80, 80, 80); // Cinza
  anim.UpdateNodeSize (buildingNode.Get (0)->GetId (), 48.0, 48.0);

  // ============================================================================
  // ETAPA 7: EXECUÇÃO DA SIMULAÇÃO
  // ============================================================================
  Simulator::Stop (Seconds (stopTime));
  Simulator::Run ();

  // Duração total do deslocamento do veículo (do início até a saída da interseção)
  double tempoViagem = 15.9; // Baseado no traço de mobilidade do Carro 1

  // Exporta o arquivo TXT nos dois caminhos (local e na pasta do cenário)
  ExportarRelatorioTxt (outputTxt, tempoViagem, beaconInterval, risRange);
  ExportarRelatorioTxt ("resultado_comunicacao.txt", tempoViagem, beaconInterval, risRange);

  // Exibe o resumo no terminal
  std::cout << "\n=======================================================\n";
  std::cout << "  SIMULAÇÃO CONCLUÍDA - RESUMO DOS RESULTADOS\n";
  std::cout << "=======================================================\n";
  std::cout << "1. Mensagens direto Carro-a-Carro (V2V):  " << g_msgDiretoCarroParaCarro << "\n";
  std::cout << "   Mensagens Carro-a-Antena (via RIS):     " << g_msgCarroParaAntena << "\n";
  std::cout << "   Tentativas Sem Sinal:                  " << g_tentativasSemSinal << "\n";
  std::cout << "2. Tempo em que o carro anda SEM SINAL:    " << g_tempoSemSinal << " s\n";
  std::cout << "   Tempo em que passa SE COMUNICANDO:      " << g_tempoComunicando << " s\n";
  std::cout << "3. Instante em que o RIS começa a funcionar: t = " << g_instanteInicioRis << " s\n";
  std::cout << "4. Arquivo de saída gerado com sucesso em:\n";
  std::cout << "   -> " << outputTxt << "\n";
  std::cout << "   -> resultado_comunicacao.txt\n";
  std::cout << "=======================================================\n\n";

  Simulator::Destroy ();
  return 0;
}
