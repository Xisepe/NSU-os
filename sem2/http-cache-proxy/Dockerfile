FROM ubuntu:latest AS builder

WORKDIR /app

RUN apt-get update && \
    apt-get install -y build-essential cmake

COPY . .

RUN mkdir build && \
    cd build && \
    cmake .. && \
    make

FROM ubuntu:latest

WORKDIR /app

COPY --from=builder /app/build/http_cache_proxy .

CMD ["./http_cache_proxy"]