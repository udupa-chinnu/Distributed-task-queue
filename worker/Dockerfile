FROM ubuntu:22.04 AS builder

# Install build tools
RUN apt-get update && \
    apt-get install -y git cmake make g++ libssl-dev && \
    rm -rf /var/lib/apt/lists/*

# Build POCO (minimal)
WORKDIR /poco
RUN git clone --branch poco-1.12.4 --depth 1 https://github.com/pocoproject/poco.git . && \
    mkdir cmake-build && cd cmake-build && \
    cmake .. -DCMAKE_BUILD_TYPE=Release -DENABLE_NET=ON && \
    make -j$(nproc) && \
    make install

# Build worker
WORKDIR /app
COPY worker/ .
RUN cmake . && make

# Runtime image
FROM ubuntu:22.04
RUN apt-get update && \
    apt-get install -y libssl-dev netcat-openbsd && \
    rm -rf /var/lib/apt/lists/*
COPY --from=builder /usr/local/lib/libPoco* /usr/local/lib/
COPY --from=builder /app/worker_node /app/
RUN ldconfig
WORKDIR /app
CMD ["sh", "-c", "while ! nc -z task-queue-server 8000; do sleep 2; done && ./worker_node"]