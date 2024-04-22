FROM ubuntu:latest

# Install C development tools
RUN apt-get update && apt-get install -y build-essential gcc net-tools

# Set working directory (adjust if necessary)
WORKDIR /app

# Copy server files 
COPY server/ /app/
COPY client/ /app/


EXPOSE 8080  




