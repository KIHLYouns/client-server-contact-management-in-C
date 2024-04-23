FROM ubuntu:latest

RUN apt-get update && apt-get install -y build-essential gcc net-tools nano git

WORKDIR /app

EXPOSE 8080  




