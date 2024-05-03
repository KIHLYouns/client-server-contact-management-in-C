GREEN := \033[0;32m
RED := \033[0;31m
BLUE := \033[0;34m
PURPLE := \033[0;35m
NC := \033[0m


help:
	@echo "\nAvailable targets:\n"
	@echo "${GREEN}start${NC}: Build the Docker image and run the container."
	@echo "${GREEN}t${NC}: Execute bash in the running container."
	@echo "${GREEN}serv${NC}: Compile and run the server code."
	@echo "${GREEN}cli${NC}: Compile and run the client code.\n"
	@echo "${YELLOW}Note:${NC} Make sure Docker is installed and running."

start:
	@echo "${GREEN}Building Docker image and running container...${NC}"
	docker build -t ubuntu_dev .
	docker run -it -p 8080:8080 -v /Users/youns/Documents/Sock:/app --name ubuntu_dev ubuntu_dev

t:
	@echo "${RED}Executing bash in the container...${NC}"
	docker exec -it ubuntu_dev bash

serv:
	@echo "${BLUE}Compiling and running server code inside the container...${NC}"
	cd server && gcc -o server server.c && ./server

cli:
	@echo "${PURPLE}Compiling and running client code inside the container...${NC}"
	cd client && gcc -o client client.c && ./client




