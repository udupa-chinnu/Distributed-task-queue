FROM ubuntu:22.04 AS builder

# Install ALL build tools
RUN apt-get update && \
    apt-get install -y git cmake make g++ libssl-dev libpq-dev && \
    rm -rf /var/lib/apt/lists/*

# Build POCO
WORKDIR /poco
RUN git clone --branch poco-1.12.4 --depth 1 https://github.com/pocoproject/poco.git . && \
    mkdir cmake-build && cd cmake-build && \
    cmake .. -DCMAKE_BUILD_TYPE=Release -DENABLE_DATA_POSTGRESQL=ON && \
    make -j$(nproc) && \
    make install

# Build server
WORKDIR /app
COPY server/ .
RUN cmake . && make

# Runtime image
FROM ubuntu:22.04
RUN apt-get update && \
    apt-get install -y libssl-dev libpq5 && \
    rm -rf /var/lib/apt/lists/*
COPY --from=builder /usr/local/lib/libPoco* /usr/local/lib/
COPY --from=builder /app/task_queue_server /app/
RUN ldconfig
WORKDIR /app
CMD ["./task_queue_server"]