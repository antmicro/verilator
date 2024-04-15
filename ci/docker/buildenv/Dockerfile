# DESCRIPTION: Dockerfile for env to build and fully test Verilator
#
# Copyright 2020 by Stefan Wallentowitz. This program is free software; you
# can redistribute it and/or modify it under the terms of either the GNU
# Lesser General Public License Version 3 or the Perl Artistic License
# Version 2.0.
# SPDX-License-Identifier: LGPL-3.0-only OR Artistic-2.0

FROM debian:bullseye

# Create the user
RUN groupadd verilator \
    && useradd -g verilator -m verilator -s /bin/bash \
    && apt-get update \
    && apt-get install --no-install-recommends -y sudo \
    && apt-get clean \
    && rm -rf /var/lib/apt/lists/* \
    && echo verilator ALL=\(root\) NOPASSWD:ALL > /etc/sudoers.d/verilator \
    && chmod 0440 /etc/sudoers.d/verilator

RUN apt-get update \
    && DEBIAN_FRONTEND=noninteractive \
       apt-get install --no-install-recommends -y \
                        autoconf \
                        bc \
                        bison \
                        build-essential \
                        ca-certificates \
                        ccache \
                        clang \
                        cmake \
                        flex \
                        gdb \
                        git \
                        gtkwave \
                        help2man \
                        libfl2 \
                        libfl-dev \
                        libgoogle-perftools-dev \
                        libsystemc \
                        libsystemc-dev \
                        perl \
                        python3 \
                        python3-pip \
                        wget \
                        z3 \
                        zlib1g \
                        zlib1g-dev \
                        locales \
                        bear \
    && apt-get clean \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /tmp

RUN cpan install -fi Parallel::Forker

RUN pip install --no-cache-dir --upgrade pip && \
    pip install --no-cache-dir libclang packaging

RUN git clone https://github.com/veripool/vcddiff.git && \
    make -C vcddiff && \
    cp -p vcddiff/vcddiff /usr/local/bin/vcddiff && \
    rm -rf vcddiff

ENV VERILATOR_AUTHOR_SITE=1

USER verilator

RUN sudo sed -i 's/^# *\(en_US.UTF-8\)/\1/' /etc/locale.gen && \
    sudo locale-gen

RUN echo "export LC_ALL=en_US.UTF-8" >> ~/.bashrc && \
    echo "export LANG=en_US.UTF-8" >> ~/.bashrc && \
    echo "export LANGUAGE=en_US.UTF-8" >> ~/.bashrc

RUN mkdir -p ~/.config/gdb && \
    echo "set auto-load safe-path /" > ~/.config/gdb/gdbinit
