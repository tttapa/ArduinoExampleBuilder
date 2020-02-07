FROM ubuntu:xenial

RUN apt-get update \
 && apt-get install -y software-properties-common \
 && add-apt-repository ppa:ubuntu-toolchain-r/test \
 && apt-get update && apt-get install -y \
        gcc-8 g++-8 \
        make \
        wget \
        libssl-dev \
 && rm -rf /var/lib/apt/lists/*
WORKDIR /tmp
RUN wget https://github.com/Kitware/CMake/releases/download/v3.15.2/cmake-3.15.2-Linux-x86_64.sh
RUN chmod +x cmake-3.15.2-Linux-x86_64.sh
RUN ./cmake-3.15.2-Linux-x86_64.sh --prefix=/usr/local --exclude-subdir --skip-license
COPY "src" "src"
COPY "lib" "lib"
COPY "CMakeLists.txt" "."
WORKDIR /tmp/build
RUN CC=gcc-8 CXX=g++-8 cmake .. -DCMAKE_BUILD_TYPE=Release
RUN make -j$(($(nproc) * 2))