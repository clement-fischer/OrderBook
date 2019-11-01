FROM ubuntu:bionic

ENV DEBIAN_FRONTEND noninteractive

WORKDIR /usr/include/

ENV BOOST_VERSION=1.71.0
ENV BOOST_VERSION_=1_71_0
ENV BOOST_ROOT=/usr/include/boost

RUN apt-get -qq update && apt-get install -q -y software-properties-common
RUN add-apt-repository ppa:ubuntu-toolchain-r/test -y
RUN apt-get -qq update && apt-get install -qy g++ gcc git wget build-essential

RUN wget --max-redirect 3 https://github.com/Kitware/CMake/releases/download/v3.15.5/cmake-3.15.5.tar.gz
RUN mkdir -p /tmp/cmake && tar zxf cmake-3.15.5.tar.gz -C /tmp/cmake --strip-components=1
RUN cd /tmp/cmake && ./bootstrap && make && make install

RUN wget --max-redirect 3 https://dl.bintray.com/boostorg/release/${BOOST_VERSION}/source/boost_${BOOST_VERSION_}.tar.gz
RUN mkdir -p /usr/include/boost && tar zxf boost_${BOOST_VERSION_}.tar.gz -C /usr/include/boost --strip-components=1
RUN cd /usr/include/boost && ./bootstrap.sh && ./b2 install
RUN echo ${BOOST_ROOT}

WORKDIR /orderbook
RUN git clone https://github.com/clement-fischer/OrderBook .
RUN cd build && cmake .. && make && ctest --verbose

WORKDIR /orderbook/bin

# docker build --tag orderbook:latest .
# docker run -it --rm --name orderbook orderbook:latest
