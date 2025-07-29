# Reuse the same POCO builder stage
FROM alpine:latest AS poco-builder
RUN apk add --no-cache git cmake make g++ openssl-dev
WORKDIR /poco
RUN git clone --branch poco-1.12.4 --depth 1 https://github.com/pocoproject/poco.git . && \
    mkdir cmake-build && cd cmake-build && \
    cmake .. -DENABLE_DATA_POSTGRESQL=OFF && \  
    cmake --build . --target install -- -j$(nproc)

# Final stage
FROM ubuntu:22.04
RUN apt-get update && apt-get install -y libssl-dev

COPY --from=poco-builder /usr/local/lib /usr/local/lib
COPY --from=poco-builder /usr/local/include /usr/local/include

WORKDIR /app
COPY . .
RUN cmake . && make

CMD ["./test_client"]  