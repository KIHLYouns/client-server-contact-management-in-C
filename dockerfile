FROM ubuntu:latest

# Install C development tools
RUN apt-get update && apt-get install -y build-essential gcc net-tools

# Set working directory (adjust if necessary)
WORKDIR /app/server

# Copy server files 
COPY server/ /app/server/

EXPOSE 8080  

# Compile the server
CMD ["gcc", "server.c", "-o", "server"]

# Start the server (add '&' if you want to run in background)
CMD ["./server"] 




