FROM ubuntu:18.04

WORKDIR /home/

RUN apt-get -y update
RUN apt-get -y install gcc
RUN apt-get -y install make
RUN apt-get -y install check
RUN apt-get -y install vim
RUN apt-get -y install pkg-config
RUN apt-get -y install clang-format
RUN apt-get -y install gdb

# Download & install cmocka
RUN apt-get -y install curl cmake
RUN curl https://cmocka.org/files/1.1/cmocka-1.1.5.tar.xz -o /root/cmocka-1.1.5.tar.xz
RUN cd /root/ && tar -xf /root/cmocka-1.1.5.tar.xz
RUN mkdir /root/cmocka-1.1.5/build
RUN cd /root/cmocka-1.1.5/build && cmake .. && make install
RUN ldconfig

RUN echo "set expandtab\nset tabstop=2\nset nu" > /root/.vimrc
