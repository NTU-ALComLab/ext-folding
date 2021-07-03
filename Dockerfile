FROM tensorflow/tensorflow:latest-gpu-py3
RUN apt update -y && DEBIAN_FRONTEND=noninteractive apt-get install -y --no-install-recommends tzdata
RUN apt install git build-essential clang bison flex \
        libreadline-dev gawk tcl-dev libffi-dev \
        graphviz xdot pkg-config python3 libboost-system-dev \
        libboost-python-dev libboost-filesystem-dev zlib1g-dev -y
COPY . .
RUN git clone https://github.com/berkeley-abc/abc.git tools/abc
RUN cd tools/abc && make -j8
RUN git clone https://github.com/YosysHQ/yosys.git tools/yosys
RUN cd tools/yosys && make -j8
RUN pip3 install --upgrade pip
RUN pip3 install -r requirements.txt
