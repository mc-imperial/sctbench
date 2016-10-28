FROM ubuntu:14.04

RUN \
  apt-get update && \
  apt-get -y install \
    libprotobuf-dev \
    protobuf-compiler \
    build-essential ipython m4 cmake && \
  apt-get clean && rm -rf /var/lib/apt/lists/* /tmp/* /var/tmp/*

COPY . /data/sctbench

RUN \
  /bin/bash -c \
  "cd /data/sctbench && \
  source env.template && \
  build_maple && \
  build_benchmarks"

CMD ["bash"]

