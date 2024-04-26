GREEN := \033[0;32m
YELLOW := \033[0;33m
NC := \033[0m

help:
	@echo "\nAvailable targets:\n"
	@echo "${GREEN}start${NC}: Build the Docker image and run the container."
	@echo "${GREEN}t${NC}: Execute bash in the running container."
	@echo "${GREEN}run_s${NC}: Compile and run the server code."
	@echo "${GREEN}run_c${NC}: Compile and run the client code.\n"
	@echo "${YELLOW}Note:${NC} Make sure Docker is installed and running."

start:
	@echo "${GREEN}Building Docker image and running container...${NC}"
	docker build -t ubuntu_dev .
	docker run -it -p 8080:8080 -v /Users/youns/Documents/Sock:/app --name ubuntu_dev ubuntu_dev

t:
	@echo "${GREEN}Executing bash in the container...${NC}"
	docker exec -it ubuntu_dev bash

run_s:
	@echo "${GREEN}Compiling and running server code inside the container...${NC}"
	cd server && gcc -o server server.c && ./server

run_c:
	@echo "${GREEN}Compiling and running client code inside the container...${NC}"
	cd client && gcc -o client client.c && ./client




