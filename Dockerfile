FROM amazonlinux:latest

RUN yum update -y && yum groupinstall -y "Development Tools"

RUN yum install -y git cmake iproute hostname sysstat traceroute docker iftop

RUN git clone https://github.com/chriskohlhoff/asio.git /asio

WORKDIR /home
COPY . /home
#RUN g++ --version
#docker run -v /Users/source/documents:/app/data -m 512m -p 80:80 linux
# need this command to test 
#docker run -v /var/run/docker.sock:/var/run/docker.sock linux
RUN g++ -std=c++20 -I/asio/asio/include -I./libraries/crow/include/ -I. main2.cpp stats.cpp docker.cpp parse.cpp -o main2 -lpthread

EXPOSE 18080
CMD ["./main2"]
