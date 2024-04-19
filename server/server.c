#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>

#define CONTACTS_FILE "contacts.txt"
#define USERS_FILE "users.txt"
#define MAX_MESSAGE_SIZE 256

typedef struct
{
    char username[20];
    char password[20];
} Login;

typedef struct
{
    char rue[30];
    char ville[20];
    char pays[20];
} Address;

typedef struct
{
    char nom[20];
    char prenom[20];
    int GSM;
    char email[30];
    Address adr;
} Contact;

// Function prototypes
void processClientRequest(int clientSockfd, char *role);
bool authenticateUser(int clientSockfd, char *role);
void addContact(Contact contact);
int sendMessage(int sockfd, const void *message, size_t length)
{
    int bytes_sent = send(sockfd, (const char *)message, length, 0);
    if (bytes_sent != length)
    {
        return -1;
    }
    return 0;
}

char *receiveMessage(int sockfd)
{
    char *buffer = malloc(MAX_MESSAGE_SIZE);
    if (buffer == NULL)
        return NULL;

    int bytes_received = recv(sockfd, buffer, MAX_MESSAGE_SIZE, 0);
    if (bytes_received <= 0)
    {
        free(buffer);
        return NULL;
    }

    buffer[bytes_received] = '\0';
    return buffer;
}

bool shouldContinue(int clientSockfd)
{
    char buffer[10];
    int bytes_received = recv(clientSockfd, buffer, sizeof(buffer), 0);
    if (bytes_received <= 0)
        return false;
    return strcmp(buffer, "RETRY") == 0;
}

int main()
{
    // 1. Create a socket
    int serverSockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSockfd == -1)
    {
        printf("Socket creation failed...\n");
        exit(0);
    }
    else
    {
        printf("Socket successfully created..\n");
    }

    // 2. Prepare the server address structure
    struct sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = inet_addr("127.0.0.1");
    serverAddress.sin_port = htons(8080);

    // 3. Bind the socket
    if (bind(serverSockfd, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) != 0)
    {
        printf("Socket bind failed...\n");
        exit(0);
    }
    else
    {
        printf("Socket successfully binded..\n");
    }

    // 4. Enable the socket to listen for connections
    if (listen(serverSockfd, 5) != 0)
    {
        printf("Listen failed...\n");
        exit(0);
    }
    else
    {
        printf("Server listening..\n");
    }

    // 5. Accept connections (loop)
    while (true)
    {
        struct sockaddr_in clientAddress;
        socklen_t len = sizeof(clientAddress);
        int clientSockfd = accept(serverSockfd, (struct sockaddr *)&clientAddress, &len);

        if (clientSockfd < 0)
        {
            printf("Server accept failed...\n");
        }
        else
        {
            printf("Server accepted the client...\n");
            // Authentication & Request Handling (Repeated)
            do
            {
                char role[10];
                if (authenticateUser(clientSockfd, role))
                {
                printf("User authenticated with role: %s\n", role);
                processClientRequest(clientSockfd, role);
                }

            } while (shouldContinue(clientSockfd));

            close(clientSockfd);
        }
    }
}

bool authenticateUser(int clientSockfd, char *role)
{
    char *response = receiveMessage(clientSockfd);
    Login loginCredentials;
    memcpy(&loginCredentials, response, sizeof(Login));
    free(response);

    FILE *usersFile = fopen(USERS_FILE, "r");
    if (usersFile == NULL)
    {
        perror("Error opening USERS_FILE");
        return false;
    }

    char line[100];
    while (fgets(line, sizeof(line), usersFile) != NULL)
    {
        char storedUsername[20], storedPassword[20], storedRole[10];
        if (sscanf(line, "%s %s %s", storedUsername, storedPassword, storedRole) == 3)
        {
            if (strcmp(storedUsername, loginCredentials.username) == 0 &&
                strcmp(storedPassword, loginCredentials.password) == 0)
            {
                strcpy(role, storedRole);
                fclose(usersFile);

                char response[20];
                sprintf(response, "1%s", role);
                write(clientSockfd, response, strlen(response));
                return true;
            }
        }
    }

    fclose(usersFile);
    write(clientSockfd, "0", 1);
    return false;
}

void processClientRequest(int clientSockfd, char *role)
{
    char message[MAX_MESSAGE_SIZE];
    int bytes_received = recv(clientSockfd, message, MAX_MESSAGE_SIZE, 0);

    if (bytes_received <= 0)
    {
        printf("Client disconnected or error.\n");
        return;
    }

    switch (message[0])
    {
    case '1': // Add contact
    {
        Contact newContact;
        if (recv(clientSockfd, &newContact, sizeof(Contact), 0) > 0)
        {
            addContact(newContact);
            break;
        }
    }
    case '2': // Search contact

        break;
        // ... cases for edit, delete, display ...
    
    }
}

void addContact(Contact contact)
{}
