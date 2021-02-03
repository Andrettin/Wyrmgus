FROM ubuntu:focal
LABEL Description="Environment to build and test Wyrmgus"
ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update
RUN apt-get -y install \
  build-essential \
  cmake \
  libboost-dev \
  liblua5.1-dev \
  qtlocation5-dev \
  libqt5qml5 \
  libsdl-mixer1.2-dev \
  libsdl1.2-dev \
  libsqlite3-dev \
  qtbase5-dev \
  qtmultimedia5-dev \
  qtpositioning5-dev \
  libtolua++5.1-dev \
;
RUN apt-get clean && rm -rf /var/lib/apt/lists/*
