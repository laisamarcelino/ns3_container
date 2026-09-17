# Guia de Execução — NS-3 e SUMO

Este repositório contém o ambiente de desenvolvimento em container Docker para simulações com o **NS-3.25** e o **SUMO (Simulation of Urban MObility)**.

---

## 1. Executando o NS-3

O `Makefile` na raiz do projeto encapsula todas as chamadas do Docker e do compilador WAF.

### 1.1 Execução Rápida via Makefile
Para compilar e executar qualquer exemplo nativo da pasta [`exemplos_tutorial_vlc/`](file:///home/laisa/ns3-25/exemplos_tutorial_vlc/):

```bash
# Executar exemplos do tutorial básico:
make run first
make run second
make run third
make run fourth
make run fifth
make run sixth
make run seventh
make run hello-simulator

# Executar exemplo de comunicação por luz visível (VLC / Li-Fi):
make run vlc_example
```

> **Dica:** Não é necessário digitar a extensão `.cc` (tanto `make run first` quanto `make run first.cc` funcionam).

#### Passando Argumentos na Linha de Comando
Você pode passar parâmetros de linha de comando com a variável `ARGS`:
```bash
make run third ARGS="--nWifi=7 --tracing=true"
make run seventh ARGS="--useIpv6=1"
```

---

### 1.2 Acesso Interativo ao Terminal do Container
Para inspecionar arquivos ou compilar manualmente pelo WAF:
```bash
# Abrir o bash dentro do container:
make init
# ou:
make bash

# Dentro do container (no diretório /opt/ns-allinone-3.25/ns-3.25):
./waf --run first
./waf --run "third --nWifi=5"
./waf --run vlc_example
```

---

### 1.3 Visualização Gráfica com NetAnim
Se o exemplo gerar um arquivo de animação `.xml`:
```bash
make netanim
```
1. No NetAnim, clique no ícone **File Open** (pasta amarela 📁 no topo esquerdo);
2. Selecione o arquivo `.xml` gerado dentro da pasta `workspace/`;
3. Pressione **Play** (▶️).

---

### 1.4 Comandos Úteis
```bash
make status          # Exibe status do Docker e lista os scripts .cc disponíveis
make clean-outputs   # Remove traces gerados (.pcap, .tr, .xml, .cwnd)
make fix-perms       # Corrige permissões de arquivos gerados no workspace
make build           # Recompila todo o NS-3 (./waf build)
make test            # Executa a suíte de testes unitários do NS-3
```

---

## 2. Executando o SUMO

O simulador de tráfego urbano **SUMO** e suas ferramentas estão instalados no container Docker.

### 2.1 Visualização Gráfica do Mapa (SUMO-GUI e Netedit)
Você pode abrir a interface gráfica do SUMO para inspecionar o mapa visualmente:
```bash
# Visualizar o mapa gerado (map.net.xml) no SUMO-GUI:
make sumo-gui

# Editar ou inspecionar o mapa visualmente no editor Netedit:
make netedit
```

---

### 2.2 Linha de Comando via Makefile
Para invocar comandos do SUMO, passe os argumentos através de `ARGS`:
```bash
# Verificar a versão instalada:
make sumo ARGS="--version"

# Executar uma simulação a partir de um arquivo de configuração (.sumocfg):
make sumo ARGS="-c /workspace/cenario.sumocfg"

# Gerar arquivo de trajetórias FCD (Floating Car Data):
make sumo ARGS="-c /workspace/cenario.sumocfg --fcd-output /workspace/fcd.xml"
```

---

### 2.2 Conversão de Trajetórias para o NS-3
Para converter os dados do SUMO em arquivo de mobilidade `.tcl` legível pelo `Ns2MobilityHelper` do NS-3:
```bash
docker compose run --rm ns3 python3 /usr/share/sumo/tools/traceExporter.py \
  --fcd-input /workspace/fcd.xml \
  --ns2mobility-output /workspace/mobilidade.tcl
```

---

## 📚 Documentação Adicional
- **Explicação passo a passo de cada exemplo:** [`exemplos_tutorial_vlc/explicacao_exemplos.md`](file:///home/laisa/ns3-25/exemplos_tutorial_vlc/explicacao_exemplos.md)
- **Fundamentos teóricos de redes e comunicações:** [`explicacoes.md`](file:///home/laisa/ns3-25/explicacoes.md)
