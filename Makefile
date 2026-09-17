# ==============================================================================
# Makefile Universal para o Simulador NS-3 (Versão 3.25)
# Funciona TANTO fora (Host) QUANTO dentro do Container Docker!
# ==============================================================================

NS3_DIR := /opt/ns-allinone-3.25/ns-3.25

# Detecta se o comando está sendo executado DENTRO ou FORA do container Docker
IS_CONTAINER := $(shell [ -f /.dockerenv ] || [ -d /opt/ns-allinone-3.25/ns-3.25/build ] && echo 1 || echo 0)

ifeq ($(IS_CONTAINER),1)
  DOCKER_CMD :=
else
  DOCKER_CMD := docker compose run --rm -w $(NS3_DIR) ns3
endif

# Captura argumentos adicionais para "make run <script> [ARGS...]"
ifeq (run,$(firstword $(MAKECMDGOALS)))
  RUN_TARGET := $(wordlist 2,2,$(MAKECMDGOALS))
  RUN_EXTRA_ARGS := $(wordlist 3,$(words $(MAKECMDGOALS)),$(MAKECMDGOALS))
  $(eval $(RUN_TARGET):;@:)
  $(foreach arg,$(RUN_EXTRA_ARGS),$(eval $(arg):;@:))
endif

RAW_TARGET := $(if $(SCRIPT),$(SCRIPT),$(RUN_TARGET))
# Remove extensão .cc se o usuário tiver digitado (ex: make run first.cc -> first)
TARGET := $(patsubst %.cc,%,$(RAW_TARGET))

.PHONY: help init shell bash run build configure clean clean-outputs test netanim fix-perms status sumo sumo-gui netedit

default: help

## help: Exibe todos os comandos disponíveis com exemplos de uso
help:
	@echo "========================================================================"
	@echo "                   🚀 Comandos do NS-3 (Makefile)                       "
	@echo "========================================================================"
	@echo "  make run <script>       - Compila e executa qualquer simulação C++"
	@echo "                            Exemplos: make run first"
	@echo "                                      make run second"
	@echo "                                      make run vlc_example"
	@echo "                                      make run third ARGS=\"--nWifi=5\""
	@echo "  make init               - Abre o terminal interativo (Bash) no NS-3"
	@echo "  make netanim            - Abre a interface gráfica NetAnim no host"
	@echo "  make sumo-gui           - Abre a interface gráfica SUMO-GUI para ver o mapa"
	@echo "  make netedit            - Abre o editor visual Netedit do SUMO"
	@echo "  make status             - Exibe o status dos containers e scripts"
	@echo "  make build              - Compila todo o código do NS-3 (./waf build)"
	@echo "  make configure          - Reconfigura o NS-3 com suporte a testes/exemplos"
	@echo "  make clean              - Limpa arquivos compilados do NS-3 (./waf clean)"
	@echo "  make clean-outputs      - Remove capturas .pcap, logs .tr e dados gerados"
	@echo "  make test               - Executa a suíte de testes do NS-3"
	@echo "  make sumo ARGS=\"...\"    - Executa comandos do simulador SUMO no container"
	@echo "  make fix-perms          - Ajusta permissões dos arquivos na pasta workspace"
	@echo "========================================================================"

## init: Abre o terminal interativo (Bash) dentro da pasta raiz do NS-3
init:
ifeq ($(IS_CONTAINER),1)
	@echo "⚠️ Você já está dentro do container NS-3!"
else
	@echo "🐳 Abrindo terminal interativo no container NS-3..."
	@docker compose run --rm -it -w $(NS3_DIR) ns3 /bin/bash
endif

shell: init
bash: init

## status: Exibe o status do Docker e lista os scripts disponíveis no workspace
status:
	@echo "=== 🐳 Status do Docker ==="
	@docker compose ps 2>/dev/null || echo "Docker Compose não ativo"
	@echo ""
	@echo "=== 📜 Scripts de Simulação Encontrados em ./workspace ==="
	@find workspace -name "*.cc" | sort | sed 's/^/  • /'

## run: Executa uma simulação (busca o script em qualquer pasta do workspace)
run:
	@if [ -z "$(TARGET)" ]; then \
		echo "❌ Erro: Especifique o nome do script!"; \
		echo "   Uso: make run <nome_do_script> [ARGS=\"--arg=val\"]"; \
		echo "   Exemplos: make run first"; \
		echo "             make run second"; \
		echo "             make run vlc_example"; \
		echo "             make run third ARGS=\"--nWifi=5\""; \
		exit 1; \
	fi; \
	echo "🚀 Executando simulação: $(TARGET)..."; \
	if [ "$(IS_CONTAINER)" = "1" ]; then \
		cd $(NS3_DIR) && \
		SRC_FILE=$$(find /workspace -name "$(TARGET).cc" 2>/dev/null | head -n 1) && \
		if [ -n "$$SRC_FILE" ]; then cp -f "$$SRC_FILE" scratch/$(TARGET).cc; fi; \
		./waf --run "$(TARGET) $(ARGS) $(RUN_EXTRA_ARGS)" && \
		cp -f *.xml *.pcap *.tr *.cwnd *.plt *.dat /workspace/ 2>/dev/null || true; \
	else \
		$(DOCKER_CMD) /bin/bash -c '\
			cd $(NS3_DIR) && \
			SRC_FILE=$$(find /workspace -name "$(TARGET).cc" 2>/dev/null | head -n 1) && \
			if [ -n "$$SRC_FILE" ]; then cp -f "$$SRC_FILE" scratch/$(TARGET).cc; fi; \
			./waf --run "$(TARGET) $(ARGS) $(RUN_EXTRA_ARGS)" && \
			cp -f *.xml *.pcap *.tr *.cwnd *.plt *.dat /workspace/ 2>/dev/null || true' && \
		docker compose run --rm ns3 chown -R $(shell id -u):$(shell id -g) /workspace 2>/dev/null || true; \
	fi

## build: Compila o projeto NS-3 usando o WAF
build:
	@echo "🔨 Compilando o NS-3..."
ifeq ($(IS_CONTAINER),1)
	@cd $(NS3_DIR) && ./waf build
else
	@$(DOCKER_CMD) ./waf build
endif

## configure: Configura o WAF com flags de compatibilidade GCC
configure:
	@echo "⚙️ Configurando o WAF..."
ifeq ($(IS_CONTAINER),1)
	@cd $(NS3_DIR) && CXXFLAGS="-w" ./waf configure --enable-examples --enable-tests
else
	@$(DOCKER_CMD) /bin/bash -c 'CXXFLAGS="-w" ./waf configure --enable-examples --enable-tests'
endif

## clean: Limpa arquivos temporários de compilação
clean:
	@echo "🧹 Limpando compilação do NS-3..."
ifeq ($(IS_CONTAINER),1)
	@cd $(NS3_DIR) && ./waf clean
else
	@$(DOCKER_CMD) ./waf clean
endif

## clean-outputs: Remove arquivos de saída e rastreamento gerados pelas simulações
clean-outputs:
	@echo "🧹 Removendo arquivos temporários de simulação (.pcap, .tr, .xml, .cwnd, .plt, .dat)..."
	@rm -f workspace/*.pcap workspace/*.tr workspace/*.xml workspace/*.cwnd workspace/*.plt workspace/*.dat
	@echo "✅ Pasta workspace limpa de arquivos de rastreamento!"

## test: Executa a suíte de testes do NS-3
test:
	@echo "🧪 Executando testes..."
ifeq ($(IS_CONTAINER),1)
	@cd $(NS3_DIR) && ./test.py
else
	@$(DOCKER_CMD) ./test.py
endif

## netanim: Abre o executável do NetAnim no computador host
netanim:
	@if [ -f ./workspace/netanim/NetAnim ]; then \
		echo "🎨 Abrindo NetAnim..."; \
		./workspace/netanim/NetAnim & \
	else \
		echo "❌ Executável do NetAnim não encontrado em ./workspace/netanim/NetAnim"; \
		exit 1; \
	fi

## fix-perms: Corrige permissões de arquivos na pasta workspace
fix-perms:
	@echo "🔧 Ajustando permissões da pasta workspace..."
	@docker compose run --rm ns3 chown -R $(shell id -u):$(shell id -g) /workspace

## sumo: Executa comandos e ferramentas do SUMO dentro do container Docker
sumo:
ifeq ($(IS_CONTAINER),1)
	@sumo $(ARGS)
else
	@docker compose run --rm ns3 sumo $(ARGS)
endif

XAUTH := $(if $(XAUTHORITY),$(XAUTHORITY),$(HOME)/.Xauthority)

## sumo-gui: Abre a interface gráfica do SUMO (SUMO-GUI)
sumo-gui:
	@xhost +local:root 2>/dev/null || true
	@docker compose run --rm \
		-e LIBGL_ALWAYS_SOFTWARE=1 \
		-e DISPLAY=$(DISPLAY) \
		-e FOXDIR=/root/.foxrc \
		-v /tmp/.X11-unix:/tmp/.X11-unix \
		-v $(XAUTH):/root/.Xauthority:ro \
		-e XAUTHORITY=/root/.Xauthority \
		-v $(PWD)/workspace/.foxrc:/root/.foxrc \
		ns3 sumo-gui -c /workspace/cenario_cruzamento/sim.sumocfg $(ARGS)

## netedit: Abre o editor visual de mapas do SUMO (Netedit)
netedit:
	@xhost +local:root 2>/dev/null || true
	@docker compose run --rm \
		-e LIBGL_ALWAYS_SOFTWARE=1 \
		-e DISPLAY=$(DISPLAY) \
		-e FOXDIR=/root/.foxrc \
		-v /tmp/.X11-unix:/tmp/.X11-unix \
		-v $(XAUTH):/root/.Xauthority:ro \
		-e XAUTHORITY=/root/.Xauthority \
		-v $(PWD)/workspace/.foxrc:/root/.foxrc \
		ns3 netedit /workspace/cenario_cruzamento/map.net.xml -a /workspace/cenario_cruzamento/obstaculos.poly.xml $(ARGS)


