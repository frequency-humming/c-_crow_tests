FROM alpine:latest

RUN apk update && apk add --no-cache \
    g++ \
    make \
    cmake \
    git    \
    linux-headers

RUN git clone https://github.com/chriskohlhoff/asio.git /asio
WORKDIR /home
COPY . /home

RUN g++ -std=c++20 -I/asio/asio/include -I./libraries/crow/include/ -I. main2.cpp stats.cpp -o main2 -lpthread

EXPOSE 18080
CMD ["./main2"]
