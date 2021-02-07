FROM ubuntu:focal
LABEL Description="Environment to build and test Wyrmgus"
ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update
RUN apt-get -y install \
  build-essential \
  cmake \
  git \
  libboost-dev \
  libbz2-dev \
  liblua5.1-dev \
  libsdl-mixer1.2-dev \
  libsdl1.2-dev \
  libsqlite3-dev \
  libtolua++5.1-dev \
  qtbase5-dev \
  qtlocation5-dev \
  qtmultimedia5-dev \
  qtpositioning5-dev \
  xvfb \
;
RUN apt-get clean && rm -rf /var/lib/apt/lists/*
