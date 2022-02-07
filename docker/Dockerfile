FROM ubuntu:20.04

RUN apt-get update -y && DEBIAN_FRONTEND=noninteractive apt-get install -y \
    build-essential \
    cmake \
    libexpat1-dev \
    liblua5.3-dev \
    libjson-c-dev \
    python3-dev \
    python3-pip \
    python3-tk \
    git


RUN pip3 install PySimpleGUI

ADD . /cfsat

RUN cd cfsat/cfe-eds-framework && \
    make SIMULATION=native prep && \
    make install

WORKDIR /cfsat/cfe-eds-framework