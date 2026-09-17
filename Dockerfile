FROM ubuntu:20.04

ENV DEBIAN_FRONTEND=noninteractive

# Instala dependências essenciais do sistema, compiladores C/C++, Python 2, GTK, GSL, SQLite e utilitários
RUN apt-get update && apt-get install -y \
    build-essential \
    gcc \
    g++ \
    make \
    python2 \
    python2-dev \
    python2-minimal \
    python-is-python2 \
    git \
    mercurial \
    wget \
    tar \
    bzip2 \
    pkg-config \
    ca-certificates \
    libsqlite3-dev \
    libxml2-dev \
    libgtk2.0-dev \
    libgsl-dev \
    libboost-all-dev \
    gnuplot \
    tcpdump \
    gdb \
    graphviz \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /opt

# Download e extração do pacote NS-3 3.25 All-in-One
RUN wget --no-check-certificate https://www.nsnam.org/release/ns-allinone-3.25.tar.bz2 && \
    tar -xjf ns-allinone-3.25.tar.bz2 && \
    rm ns-allinone-3.25.tar.bz2

WORKDIR /opt/ns-allinone-3.25/ns-3.25

# Download e inclusão do módulo VLC (Visible Light Communication)
RUN git clone https://github.com/Aldalbahias/VLC-ns3-v3.25.git src/vlc && \
    sed -i "s/\['core'\]/\['core', 'network', 'mobility', 'point-to-point', 'propagation'\]/g" src/vlc/wscript && \
    sed -i 's/"ns3\/visible-light-communication-helper.h"/"ns3\/vlc-module.h"/g' src/vlc/examples/vlc-example.cc && \
    sed -i 's/"ns3\/visible-light-communication.h"/"ns3\/vlc-module.h"/g' src/vlc/test/vlc-test-suite.cc && \
    printf '# -*- Mode: python; py-indent-offset: 4; indent-tabs-mode: nil; coding: utf-8; -*-\n\ndef build(bld):\n    obj = bld.create_ns3_program("vlc-example", ["vlc", "core"])\n    obj.source = "vlc-example.cc"\n    obj2 = bld.create_ns3_program("vlc_example", ["vlc", "core", "internet", "applications", "network", "netanim"])\n    obj2.source = "vlc_example.cc"\n' > src/vlc/examples/wscript

# Configuração e compilação do NS-3.25 com todos os módulos e exemplos (incluindo VLC)
# CXXFLAGS="-w" suprime alertas estritos do GCC 9 (ex: -Wformat-overflow no módulo WiMAX) que causavam erro com -Werror
RUN CXXFLAGS="-w" ./waf configure --enable-examples --enable-tests && \
    ./waf build

# Configuração de variáveis de ambiente para facilitar o uso direto do NS-3 e bindings Python
ENV NS3_HOME=/opt/ns-allinone-3.25/ns-3.25
ENV PATH="/opt/ns-allinone-3.25/ns-3.25:${PATH}"
ENV PYTHONPATH="/opt/ns-allinone-3.25/ns-3.25/build/bindings/python:${PYTHONPATH}"
ENV LD_LIBRARY_PATH="/opt/ns-allinone-3.25/ns-3.25/build:${LD_LIBRARY_PATH}"

# Instalação do SUMO (Simulation of Urban MObility) e ferramentas de mobilidade veicular
RUN apt-get update && apt-get install -y \
    sumo \
    sumo-tools \
    sumo-doc \
    && rm -rf /var/lib/apt/lists/*

ENV SUMO_HOME=/usr/share/sumo
ENV PATH="${SUMO_HOME}/bin:${SUMO_HOME}/tools:${PATH}"

WORKDIR /workspace

CMD ["/bin/bash"]