FROM ubuntu:18.04

WORKDIR /home/

RUN apt-get -y update
RUN apt-get -y install gcc
RUN apt-get -y install make
RUN apt-get -y install check
RUN apt-get -y install vim
RUN apt-get -y install pkg-config

CMD ls /home/
