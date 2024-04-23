FROM ubuntu:latest

RUN apt-get update && apt-get install -y build-essential gcc net-tools

WORKDIR /app
 
COPY server/ /app/
COPY client/ /app/

EXPOSE 8080  




