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
bool authenticateUser(int clientSockfd, char *role);
bool processClientRequest(int clientSockfd, char *role);
void addContact(Contact contact, int clientSockfd);
void searchContact(char *contactName, int clientSockfd);

int sendMessage(int sockfd, const void *message, size_t length);
char *receiveMessage(int sockfd);

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
    int bindResult;
    do
    {
        bindResult = bind(serverSockfd, (struct sockaddr *)&serverAddress, sizeof(serverAddress));
        if (bindResult != 0)
        {
            perror("Socket bind failed");
            sleep(1); // Attendre 1 seconde avant la prochaine tentative
        }
    } while (bindResult != 0);

    printf("Socket successfully binded..\n");

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
            char role[10];
            while (authenticateUser(clientSockfd, role))
            {
                printf("User authenticated with role: %s\n", role);
                while (processClientRequest(clientSockfd, role))
                {
                    // Just loop until the client breaks the connection
                }
                printf("Client disconnected...\n");
            }
        }
    }
}

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

bool authenticateUser(int clientSockfd, char *role)
{
    char response[20];
    do
    {
        char *credentials = receiveMessage(clientSockfd);

        if (credentials == NULL)
        {
            return false;
        }

        Login loginCredentials;
        memcpy(&loginCredentials, credentials, sizeof(Login));

        FILE *usersFile = fopen(USERS_FILE, "r");
        if (usersFile == NULL)
        {
            perror("Error opening USERS_FILE");
            sprintf(response, "0");
            if (sendMessage(clientSockfd, response, strlen(response)) == -1)
            {
                continue;
            }
            continue;
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
                    sprintf(response, "1%s", role);
                    fclose(usersFile);
                    if (sendMessage(clientSockfd, response, strlen(response)) == -1)
                    {
                        continue;
                    }
                    return true;
                }
            }
        }

        fclose(usersFile);
        sprintf(response, "0");
        if (sendMessage(clientSockfd, response, strlen(response)) == -1)
        {
            continue;
        }
    } while (true);
}


bool processClientRequest(int clientSockfd, char *role)
{
    char *message = receiveMessage(clientSockfd);

    if (message == NULL)
    {
        return false;
    }
    else
    {
        printf("Received message: %s\n", message);
    }

    switch (message[0])
    {
    case '1': // Add contact
    {
        Contact newContact;
        memcpy(&newContact, message + 1, sizeof(Contact));
        addContact(newContact, clientSockfd);
        break;
    }
    case '2': // search contact by name
    {
        char contactName[20];
        strncpy(contactName, message + 1, strlen(message) - 1);
        contactName[strlen(message) - 1] = '\0';
        searchContact(contactName, clientSockfd);
        break;
    }
    case '3':
        break;
    case '4':
        break;
    case '5':
        break;
    case '0':
        return false;
    }
    return true;
}

void addContact(Contact contact, int clientSockfd)
{
    FILE *contactsFile = fopen(CONTACTS_FILE, "a"); // Open in append mode
    if (contactsFile == NULL)
    {
        perror("Error opening contacts.txt");
        sendMessage(clientSockfd, "0", 2);
        printf("Error message sent\n");
        return;
    }

    // Format Contact Data (Change as needed based on storage method)
    fprintf(contactsFile, "%s#%s#%d#%s#%s#%s#%s\n",
            contact.nom, contact.prenom, contact.GSM, contact.email,
            contact.adr.rue, contact.adr.ville, contact.adr.pays);

    fclose(contactsFile);

    printf("Contact added successfully:\n");
    printf("%s#%s#%d#%s#%s#%s#%s\n",
           contact.nom, contact.prenom, contact.GSM, contact.email,
           contact.adr.rue, contact.adr.ville, contact.adr.pays);

    sendMessage(clientSockfd, "1", 2);
    printf("Succes message sent\n");
}

void searchContact(char *contactName, int clientSockfd)
{
    // Read contacts from file
    FILE *contactsFile = fopen(CONTACTS_FILE, "r");
    if (contactsFile == NULL)
    {
        perror("Error opening contacts.txt");
        sendMessage(clientSockfd, "0", 2);
        printf("Error message sent\n");
        return;
    }

    int found = 0;
    Contact contact;
    char buffer[100];
    int lineNumber = 0;
    while (fgets(buffer, sizeof(buffer), contactsFile) != NULL)
    {
        lineNumber++;
        printf("Reading line %d\n", lineNumber);

        // skip empty lines
        if (buffer[0] == '\n')
            continue;

        char *firstName = strtok(buffer, "#");
        if (firstName == NULL)
        {
            printf("Found empty line\n");
            continue;
        }

        if (strcmp(firstName, contactName) == 0)
        {
            found = 1;

            // Extract other fields
            char *token = strtok(NULL, "#");
            strncpy(contact.prenom, token, sizeof(contact.prenom));

            token = strtok(NULL, "#");
            sscanf(token, "%d", &contact.GSM);

            token = strtok(NULL, "#");
            strncpy(contact.email, token, sizeof(contact.email));

            token = strtok(NULL, "#");
            strncpy(contact.adr.rue, token, sizeof(contact.adr.rue));

            token = strtok(NULL, "#");
            strncpy(contact.adr.ville, token, sizeof(contact.adr.ville));

            token = strtok(NULL, "#");
            strncpy(contact.adr.pays, token, sizeof(contact.adr.pays));

            strncpy(contact.nom, firstName, sizeof(contact.nom));
        }
    }

    if (found)
    {
        sendMessage(clientSockfd, (const void *)&contact, sizeof(Contact));
        printf("Contact details sent\n");
    }
    else
    {
        printf("Contact %s not found\n", contactName);
        sendMessage(clientSockfd, "0", 2);
    }

    fclose(contactsFile);
}
