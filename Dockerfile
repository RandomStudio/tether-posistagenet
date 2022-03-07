FROM ubuntu:20.04
WORKDIR /app
COPY build .
COPY so /usr/lib/x86_64-linux-gnu
RUN apt update && apt install -y openssl libc6-dev
