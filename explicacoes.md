# 📖 Fundamentos de Redes e Sistemas de Comunicações

Este documento serve como referência teórica e conceitual sobre os princípios de **Redes de Computadores**, **Sistemas de Comunicação Sem Fio**, **Comunicações Ópticas (VLC / Li-Fi)** e **Comunicações Veiculares Avançadas com Metasuperfícies (RIS - 6G)**.

---

## 📑 Sumário

1. [Arquitetura de Redes e Simulação de Eventos Discretos](#1-arquitetura-de-redes-e-simulação-de-eventos-discretos)
2. [Camada de Enlace e Meios Físicos de Transmissão](#2-camada-de-enlace-e-meios-físicos-de-transmissão)
3. [Camada de Rede: Endereçamento e Roteamento](#3-camada-de-rede-endereçamento-e-roteamento)
4. [Camada de Transporte: UDP e TCP](#4-camada-de-transporte-udp-e-tcp)
5. [Comunicações Ópticas Sem Fio (VLC / Li-Fi)](#5-comunicações-ópticas-sem-fio-vlc--li-fi)
6. [Comunicações Veiculares (V2V/V2X) e Superfícies Refletoras Inteligentes (RIS)](#6-comunicações-veiculares-v2vv2x-e-superfícies-refletoras-inteligentes-ris)
7. [Métricas Científicas de Desempenho em Redes](#7-métricas-científicas-de-desempenho-em-redes)

---

## 1. Arquitetura de Redes e Simulação de Eventos Discretos

### 1.1 Modelo em Camadas (OSI e TCP/IP)
Redes de computadores modernas utilizam o princípio de modularidade e isolamento de responsabilidades através de pilhas de protocolos:

```text
    Modelo OSI (7 Camadas)                 Modelo TCP/IP (4/5 Camadas)
  ┌─────────────────────────┐             ┌─────────────────────────┐
  │  7. Aplicação           │ ──────────► │  Aplicação              │ (HTTP, DNS, Echo)
  │  6. Apresentação        │             │                         │
  │  5. Sessão              │             └────────────┬────────────┘
  ├─────────────────────────┤                          │
  │  4. Transporte          │ ──────────► ┌────────────▼────────────┐
  │                         │             │  Transporte (TCP / UDP) │
  ├─────────────────────────┤             └────────────┬────────────┘
  │  3. Rede                │ ──────────► ┌────────────▼────────────┐
  │                         │             │  Rede / Internet (IP)   │
  ├─────────────────────────┤             └────────────┬────────────┘
  │  2. Enlace de Dados     │ ──────────► ┌────────────▼────────────┐
  │  1. Física              │             │  Enlace e Física (MAC)  │
  └─────────────────────────┘             └─────────────────────────┘
```

- **Encapsulamento**: Conforme os dados descem da camada de aplicação até a física, cada camada adiciona seu próprio cabeçalho (*Header*), formando a Unidade de Dados de Protocolo (*Protocol Data Unit - PDU*):
  - *Mensagem* (Aplicação) $\to$ *Segmento/Datagrama* (Transporte) $\to$ *Pacote* (Rede) $\to$ *Quadro/Frame* (Enlace) $\to$ *Bits* (Física).

### 1.2 O Paradigma da Simulação de Eventos Discretos (DES)
Em sistemas de telecomunicações, o tempo pode ser modelado de forma contínua ou discreta:
- **Tempo Contínuo**: Avalia equações diferenciais a cada instante infinitesimal ($\Delta t \to 0$). Ineficiente para redes onde a maior parte do tempo o meio está ocioso.
- **Simulação de Eventos Discretos (*Discrete-Event Simulation - DES*)**: O estado do sistema só muda quando ocorre um **evento** específico (ex: término de transmissão, timeout de pacote, expiração de temporizador).

```text
  Linha do Tempo Virtual (Simulation Clock)
  ────────────────────────────────────────────────────────────────────────►
  t = 1.0s             t = 1.002s            t = 2.0s           t = 2.005s
  [Inicia Servidor]    [Nó 0 gera pacote]    [Nó 0 envia pacote] [Nó 1 recebe]
          ▲                    ▲                     ▲                  ▲
          └────── Salto ───────┴─────── Salto ───────┴────── Salto ─────┘
                 (Intervalos ociosos não consomem tempo de processamento)
```

O simulador mantém uma fila de prioridade ordenada cronologicamente por timestamp. O relógio virtual "salta" diretamente para o próximo evento, garantindo alta eficiência computacional.

### 1.3 Entidades Fundamentais em Redes Simuladas
1. **Nó (*Node*)**: O hospedeiro (*host*) computacional, capaz de executar processos de software e acoplar interfaces físicas.
2. **Interface de Rede (*NetDevice / NIC*)**: Responsável pelo enquadramento de dados de camada 2 e controle de acesso ao meio.
3. **Canal de Comunicação (*Channel*)**: Representação do meio físico compartilhado ou dedicado (cobre, fibra óptica, ar).
4. **Pacote (*Packet*)**: Buffer de bytes contendo dados úteis e tags de controle manipuladas pelas camadas da rede.
5. **Aplicação (*Application*)**: Origem ou sumidouro (*sink*) de tráfego de dados.

---

## 2. Camada de Enlace e Meios Físicos de Transmissão

### 2.1 Enlaces Ponto a Ponto (Point-to-Point)
Em um enlace serial dedicado conectando exatamente dois nós:
- **Taxa de Transmissão / Largura de Banda ($R$)**: Capacidade de saída do transmissor em bits por segundo (bps).
- **Atraso de Transmissão ($T_{tx}$)**: Tempo necessário para empurrar todos os bits do pacote para o canal:
  $$T_{tx} = \frac{L}{R}$$
  onde $L$ é o tamanho do pacote em bits e $R$ é a taxa em bps.
- **Atraso de Propagação ($T_{prop}$)**: Tempo que o primeiro bit leva para percorrer o meio físico da origem ao destino:
  $$T_{prop} = \frac{d}{v}$$
  onde $d$ é a distância física do enlace e $v$ é a velocidade de propagação do sinal no meio ($v \approx 2 \times 10^8\text{ m/s}$ no cobre/fibra ou $3 \times 10^8\text{ m/s}$ no vácuo/ar).

### 2.2 Redes em Barramento Compartilhado e Acesso ao Meio (MAC)
Quando múltiplos nós compartilham o mesmo meio de transmissão, transmissões simultâneas colidem, tornando o sinal ininteligível.
- **Domínio de Colisão**: Conjunto de nós onde a transmissão simultânea de dois nós resulta em corrupção de quadros.
- **CSMA (Carrier Sense Multiple Access)**:
  - *Ouvir antes de falar*: O transmissor escuta o canal. Se estiver ocupado, adia a transmissão.
  - **CSMA/CD (Collision Detection)**: Utilizado em redes cabeadas (Ethernet IEEE 802.3). Se detectada colisão durante o envio, aborta imediatamente e aplica espera com recuo exponencial (*exponential backoff*).
  - **CSMA/CA (Collision Avoidance)**: Utilizado em redes sem fio (IEEE 802.11). Como os nós não conseguem ouvir enquanto transmitem na mesma frequência (diferença de potência entre sinal próprio e recebido), utilizam confirmações obrigatórias (ACKs) e temporizadores DIFS/SIFS com backoff preventivo.

### 2.3 Redes Sem Fio (Wireless / IEEE 802.11)
- **Topologia em Infraestrutura**:
  - **Ponto de Acesso (*Access Point - AP*)**: Estação central coordenadora conectada à rede cabeada.
  - **Estações Clientes (*Station - STA*)**: Nós clientes que se associam ao AP através de um Identificador de Conjunto de Serviços (*SSID*).
- **Modelos de Perda de Propagação no Canal Sem Fio**:
  - **Espaço Livre (Friis)**: A atenuação cresce com o quadrado da distância e da frequência:
    $$P_{rx} = P_{tx} \cdot G_{tx} \cdot G_{rx} \left(\frac{\lambda}{4\pi d}\right)^2$$
  - **Desvanecimento (*Fading*)**: Variações rápidas ou lentas de potência causadas por múltiplos percursos (*Multipath*) e sombreamento (*Shadowing*).
- **Modelos de Mobilidade**:
  - **Estática**: Coordenadas imutáveis $(x, y, z)$.
  - **Estocástica (Random Walk / Random Waypoint)**: Nós se deslocam para destinos e velocidades aleatórias.
  - **Traços Realistas (Mobilidade Urbana / Veicular)**: Trajetórias importadas de ferramentas especializadas (ex: SUMO).

---

## 3. Camada de Rede: Endereçamento e Roteamento

A camada de rede é responsável pela entrega fim-a-fim de datagramas entre hospedeiros localizados em redes distintas.

```text
       Sub-rede A (10.1.1.0/24)                      Sub-rede B (10.1.2.0/24)
  [ Host 1 ] ──────────── [ Roteador / Gateway ] ════════════ [ Host 2 ]
  10.1.1.1                10.1.1.2    10.1.2.1                10.1.2.2
```

### 3.1 Endereçamento IPv4 vs IPv6
- **IPv4 (32 bits)**:
  - Notação decimal com pontos (ex: `192.168.1.1`).
  - Máscara de Sub-rede e CIDR (ex: `/24` indica 24 bits de rede e 8 bits de host, permitindo até 254 endereços úteis).
  - Limite de $\approx 4{,}3$ bilhões de endereços (esgotado comercialmente).
- **IPv6 (128 bits)**:
  - Notação hexadecimal com dois pontos (ex: `2001:0db8:85a3:0000:0000:8a2e:0370:7334`).
  - Espaço de endereçamento de $2^{128} \approx 3{,}4 \times 10^{38}$ endereços.
  - Elimina a necessidade de NAT (Network Address Translation) e incorpora segurança nativa (IPsec) e autoconfiguração sem estado (SLAAC).

### 3.2 Roteamento e Algoritmo de Dijkstra
- **Tabela de Repasse (*Forwarding Table*)**: Mapeia o prefixo de rede de destino para a interface de saída física e o próximo salto (*Next Hop*).
- **Algoritmo de Dijkstra (Shortest Path First - SPF)**:
  - Modela a rede como um grafo direcionado ponderado $G = (V, E)$, onde os nós são vértices e os enlaces são arestas com custos associados (inverso da largura de banda, latência métrica, contagem de saltos).
  - Calcula a árvore de caminhos mínimos a partir da raiz até todos os nós destinos com complexidade assintótica $O(|E| + |V| \log |V|)$.

---

## 4. Camada de Transporte: UDP e TCP

A camada de transporte fornece comunicação lógica entre processos de aplicação rodando em hospedeiros diferentes, multiplexados via portas numéricas (16 bits: 0 a 65535).

```text
                     Pilha de Transporte
          ┌───────────────────────────────────────┐
          │        Aplicações de Usuário          │
          └───────────┬───────────────┬───────────┘
                      │               │
        ┌─────────────▼─────────┐   ┌─▼─────────────────────┐
        │   UDP (Não Confiável) │   │  TCP (Confiável/Fluxo)│
        │ • Sem Handshake       │   │ • 3-Way Handshake     │
        │ • Sem Retransmissão   │   │ • Controle de Fluxo   │
        │ • Baixíssima Latência │   │ • Controle de Congest.│
        └───────────────────────┘   └───────────────────────┘
```

### 4.1 Protocolo UDP (User Datagram Protocol)
- **Características**: Não orientado a conexão, sem estado (*stateless*), sem confirmação de recebimento.
- **Cabeçalho Mínimo**: Apenas 8 bytes (Porta Origem, Porta Destino, Comprimento e Checksum).
- **Casos de Uso**: Transmissões sensíveis a atraso onde perdas pontuais são toleráveis (voz sobre IP, streaming de vídeo ao vivo, beacons de segurança veicular, broadcast de rede).

### 4.2 Protocolo TCP (Transmission Control Protocol)
- **Confiabilidade Total**: Entrega em ordem e sem perdas para a aplicação através de números de sequência e reconhecimentos cumulativos (ACKs).
- **Conexão (*Three-Way Handshake*)**:
  1. Cliente envia `SYN` (sincronização de sequência inicial).
  2. Servidor responde com `SYN-ACK`.
  3. Cliente responde com `ACK`, estabelecendo o canal bidirecional.
- **Controle de Fluxo**:
  - O receptor informa na janela anunciada (*Advertised Window*) a quantidade de bytes livres em seu buffer, impedindo que um transmissor rápido sobrecarregue um receptor lento.
- **Controle de Congestionamento**:
  - Evita que múltiplos transmissores colapsem a capacidade dos roteadores intermediários da rede.
  - **Janela de Congestionamento (`cwnd`)**: Limite dinâmico de bytes que o transmissor pode injetar na rede sem receber ACKs.
  - **Slow Start**: O tamanho de `cwnd` inicia baixo e dobra a cada RTT (crescimento exponencial).
  - **Evitação de Congestionamento (AIMD - Additive Increase Multiplicative Decrease)**: Após atingir o limiar (*ssthresh*), a janela cresce linearmente (+1 MSS por RTT). Se houver perda, a janela é reduzida pela metade (ou volta a 1 MSS em timeout).

---

## 5. Comunicações Ópticas Sem Fio (VLC / Li-Fi)

A tecnologia **VLC (Visible Light Communication)** ou **Li-Fi (Light Fidelity)** utiliza o espectro da luz visível para transmitir dados em alta velocidade.

```text
                               Espectro Eletromagnético
  ... │ Micro-ondas (Wi-Fi/4G/5G) │ Infravermelho │ LUZ VISÍVEL (380 a 780 nm) │ Ultravioleta │ ...
                                                  │ Frequências: 400 a 790 THz │
```

### 5.1 Vantagens e Características Físicas
- **Banda Ultra-ampla**: Faixa de frequência na ordem de centenas de Terahertz, completamente livre de regulamentação e taxas de espectro.
- **Imunidade Eletromagnética (EMI)**: Não interfere em equipamentos médicos hospitalares nem aviônicos de bordo.
- **Segurança Física**: Ondas ópticas não atravessam paredes opacas de concreto, confinando a rede à sala do emissor.

### 5.2 Modelagem do Canal Óptico
1. **Emissor LED (Padrão Lambertiano)**:
   A distribuição espacial da intensidade emitida obedece ao padrão Lambertiano generalizado:
   $$R(\phi) = \frac{m + 1}{2\pi} \cos^m(\phi)$$
   onde $\phi$ é o ângulo de emissão em relação ao eixo normal do LED e $m$ é a ordem de emissão calculada pelo ângulo de meia-potência ($\Phi_{1/2}$):
   $$m = \frac{-\ln(2)}{\ln(\cos(\Phi_{1/2}))}$$

2. **Receptor Fotodetector (Fotodiodo - PD)**:
   - Modela a área física ativa de captação de fótons ($A$).
   - **Campo de Visão (*Field of View* - FOV $\Psi_c$)**: Se o feixe incidir com ângulo $\psi > \Psi_c$, a potência captada cai para zero:
     $$T_s(\psi) g(\psi) = \begin{cases} \frac{n_r^2}{\sin^2(\Psi_c)}, & 0 \le \psi \le \Psi_c \\ 0, & \psi > \Psi_c \end{cases}$$
     onde $n_r$ é o índice de refração do concentrador óptico não-imageador (CPC).

3. **Modulações Ópticas Digitais**:
   - **OOK (On-Off Keying)**: Modulação direta liga/desliga.
   - **VPPM (Variable Pulse Position Modulation)**: Especificada pelo padrão IEEE 802.15.7; ajusta a largura de ciclo ativo para manter o nível de dimerização de luminosidade constante enquanto transporta dados.
   - **PAM / QAM Óptico**: Modulações multinível para enlaces de alto throughput.

---

## 6. Comunicações Veiculares (V2V/V2X) e Superfícies Refletoras Inteligentes (RIS)

### 6.1 O Desafio das Redes Veiculares (V2X)
A comunicação entre veículos (*Vehicle-to-Vehicle - V2V*) e com a infraestrutura rodoviária (*Vehicle-to-Infrastructure - V2I*) exige **latências ultra-baixas ($< 10\text{ ms}$)** e **altíssima confiabilidade** para troca de mensagens críticas de segurança (*Basic Safety Messages - BSM*).

Em ambientes urbanos densos, cruzamentos em ângulo reto apresentam o problema crítico de **Bloqueio NLoS (Non-Line-of-Sight)**:
- Prédios de alvenaria atenuam sinais de rádio (WAVE 5.9 GHz e mmWave) em mais de $30\text{ a }50\text{ dB}$.
- Veículos aproximando-se do cruzamento por vias ortogonais não conseguem estabelecer comunicação direta até estarem a poucos metros da colisão.

```text
                             |       |
                             |       |  Carro 2 (RX)
                             |       |  ▲ Aproximação
                             |       |  |
    ─────────────────────────+       +─────────────────────────
                                 [RIS] Metasuperfície na Esquina
                              \
          Edifício             \  Feixe refletido contorna o prédio
       [Obstáculo NLoS]         \
                                 ▼
    ─────────────────────────+       +─────────────────────────
      Carro 1 (TX)           |       |
      ────────────────►      |       |
      ▶ Aproximação          |       |
                             |       |
```

### 6.2 Superfícies Refletoras Inteligentes (RIS - 6G)
Uma **RIS (Reconfigurable Intelligent Surface)** — também conhecida como **IRS (Intelligent Reflecting Surface)** — é uma matriz planar composta por centenas ou milhares de metamateriais semicondutores quase passivos.
- **Quebra da Lei de Snell**: Em paredes convencionais, o ângulo de reflexão é igual ao de incidência ($\theta_r = \theta_i$). Na RIS, cada micro-elemento induz um deslocamento de fase eletronicamente configurável ($\theta_n \in [0, 2\pi]$).
- **Reflexão Anômala (*Beam Steering*)**: Ao aplicar um gradiente linear de fase na superfície, a RIS redireciona a frente de onda incidente em qualquer ângulo desejado, criando uma **Linha de Visada Virtual (Virtual LoS)** para contornar o edifício.

### 6.3 Modelo de Canal RIS em Cascata
O sinal assistido pela RIS percorre dois enlaces acoplados ($Carro_1 \to RIS$ e $RIS \to Carro_2$). Quando as fases de todos os $N$ elementos refletores são alinhadas construtivamente:
$$P_{rx} \propto P_{tx} \cdot N^2 \cdot \frac{\lambda^4}{(4\pi)^4 d_1^2 d_2^2}$$
- **Ganho de Feixe Quadrático ($N^2$)**: Dobrar o número de elementos refletores da superfície quadruplica ($+6\text{ dB}$) a potência recebida no destino.

---

## 7. Métricas Científicas de Desempenho em Redes

Para avaliar quantitativamente a qualidade de serviço (*QoS*) de qualquer protocolo de rede:

| Métrica | Definição Formal | Unidade Típica | Impacto na Experiência do Usuário |
| :--- | :--- | :---: | :--- |
| **Throughput (Vazão)** | Quantidade total de bits entregues com sucesso à camada de aplicação por segundo | $\text{Mbps}$ ou $\text{Gbps}$ | Velocidade de download e transferência de dados |
| **End-to-End Delay (Latência)** | Tempo total entre a transmissão do pacote na origem e sua entrega no destino | $\text{ms}$ | Responsividade em jogos, telecirurgias e veículos autônomos |
| **Jitter (Variação de Atraso)** | Desvio padrão da latência entre pacotes sucessivos de um mesmo fluxo | $\text{ms}$ | Degradação de chamadas de voz e vídeo contínuo |
| **Packet Delivery Ratio (PDR)** | Razão percentual: $(\text{Pacotes Recebidos} / \text{Pacotes Transmitidos}) \times 100$ | $\%$ | Confiabilidade do meio de comunicação |
| **Packet Loss (Perda de Pacotes)** | Quantidade de pacotes descartados por colisões, estouro de filas ou atenuação | Pacotes ou $\%$ | Necessidade de retransmissões no TCP e travamentos no UDP |

### Componentes da Latência Total
$$\text{Latência Fim-a-Fim} = \sum_{i=1}^{\text{saltos}} \left( T_{tx, i} + T_{prop, i} + T_{fila, i} + T_{proc, i} \right)$$
- **$T_{tx}$**: Atraso de transmissão na placa de rede.
- **$T_{prop}$**: Atraso de propagação da onda no meio físico.
- **$T_{fila}$**: Tempo de espera nos buffers dos roteadores intermediários.
- **$T_{proc}$**: Tempo para inspecionar cabeçalhos de pacotes e consultar tabelas de rotas.

---

> 💡 **Para ver esses conceitos aplicados em códigos executáveis:**  
> Consulte o arquivo [`explicacao_exemplos.md`](file:///home/laisa/ns3-25/exemplos_tutorial_vlc/explicacao_exemplos.md) localizado na pasta [`exemplos_tutorial_vlc/`](file:///home/laisa/ns3-25/exemplos_tutorial_vlc/).
