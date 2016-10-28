FROM ubuntu:14.04

RUN \
  apt-get update && \
  apt-get -y install \
    libprotobuf-dev \
    protobuf-compiler \
#    libprotoc-dev \
    build-essential ipython m4 cmake && \
  apt-get clean && rm -rf /var/lib/apt/lists/* /tmp/* /var/tmp/*

COPY . /data/sctbench

RUN \
  /bin/bash -c "cd /data/sctbench && \
  source env.template && \
  cd maple && \
  make compiletype=release && \
  cd .. && \
  cd benchmarks && \
  cd chess && \
  ./build.sh && \
  cd .. && \
  cd conc-bugs && \
  ./build.sh && \
  cd .. && \
  cd concurrent-software-benchmarks && \
  ./build.sh && \
  cd .. && \
  cd inspect_benchmarks && \
  ./build.sh && \
  cd .. && \
  cd inspect_examples && \
  ./build.sh && \
  cd .. && \
  cd parsec-2.0 && \
  ./buildall.sh && \
  cd .. && \
  cd safestack && \
  ./build.sh && \
  cd .. && \
  cd splash2 && \
  ./build.sh && \
  cd .."


CMD ["bash"]

