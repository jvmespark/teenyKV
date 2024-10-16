FROM ubuntu:20.04

RUN apt-get update && \
    apt-get install -y build-essential cmake libboost-system-dev libleveldb-dev libyaml-cpp-dev && \
    apt-get clean

WORKDIR /app

COPY . .

RUN cmake . && make

EXPOSE 8080

CMD ["./TeenyKV"]
