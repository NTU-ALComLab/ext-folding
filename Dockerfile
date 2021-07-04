FROM ubuntu:latest
RUN apt update -y && DEBIAN_FRONTEND=noninteractive apt-get install -y --no-install-recommends tzdata
RUN apt install git build-essential clang bison flex \
        libreadline-dev gawk tcl-dev libffi-dev \
        graphviz xdot pkg-config python3 libboost-system-dev \
        libboost-python-dev libboost-filesystem-dev zlib1g-dev -y
COPY . ext-folding
RUN git clone https://github.com/berkeley-abc/abc.git abc
RUN ln -s /ext-folding /abc/src/.
RUN cd abc && make -j8
