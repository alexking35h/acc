FROM ubuntu:focal

WORKDIR /home/

ARG DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get -y install gcc
RUN apt-get update && apt-get -y install make
RUN apt-get update && apt-get -y install check
RUN apt-get update && apt-get -y install vim
RUN apt-get update && apt-get -y install pkg-config
RUN apt-get update && apt-get -y install clang-format
RUN apt-get update && apt-get -y install gdb
RUN apt-get update && apt-get -y install git
RUN apt-get update && apt-get -y install python3
RUN apt-get update && apt-get -y install python3-pip

RUN pip3 install black
RUN pip3 install pytest

# Download & install cmocka
RUN apt-get update && apt-get -y install curl cmake
RUN curl https://cmocka.org/files/1.1/cmocka-1.1.5.tar.xz -o /root/cmocka-1.1.5.tar.xz
RUN cd /root/ && tar -xf /root/cmocka-1.1.5.tar.xz
RUN mkdir /root/cmocka-1.1.5/build
RUN cd /root/cmocka-1.1.5/build && cmake .. && make install
RUN ldconfig

RUN echo "set expandtab\nset tabstop=2\nset nu" > /root/.vimrc

# Support for cross-compiling 32-bit i386
RUN apt-get update && apt-get -y install crossbuild-essential-i386
RUN apt-get update && apt-get -y install libc6-i386

# Support for cross-compiling 32-bit ARM
RUN apt-get update && apt-get -y install gcc-8-arm-linux-gnueabi
