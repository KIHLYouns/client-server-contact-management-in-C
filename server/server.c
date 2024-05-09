#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <pthread.h>
#include <time.h>

#define CONTACTS_FILE "contacts.txt"
#define USERS_FILE "users.txt"
#define MAX_MESSAGE_SIZE 1024

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
void *handleClient(void *clientSockfd_void);
void addContact(Contact contact, int clientSockfd);
void searchContact(char *contactName, int clientSockfd);
void editContact(char *contactName, int clientSockfd);
void displayAllContacts(int clientSockfd);

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
    serverAddress.sin_addr.s_addr = htonl(INADDR_ANY);
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
            // Handle the client in a new thread
            pthread_t client_thread;
            pthread_create(&client_thread, NULL, handleClient, (void *)(long)clientSockfd);
        }
    }
}

void *handleClient(void *clientSockfd_void)
{
    // Convert the clientSockfd_void to int
    int clientSockfd = (int)(long)clientSockfd_void;

    // Authentication & Request Handling (Repeated)
    char role[10];
    while (authenticateUser(clientSockfd, role))
    {
        while (processClientRequest(clientSockfd, role))
        {
            // Just loop until the client breaks the connection
        }
        printf("Client disconnected...\n");
    }

    // Close the client socket
    close(clientSockfd);

    // Exit the thread
    pthread_exit(0);
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
                    printf("[%s = %s] Authenticated by client [%d]\n", storedUsername, role, clientSockfd);
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
        time_t rawtime;
        struct tm * timeinfo;
        char buffer[80];

        time(&rawtime);
        timeinfo = localtime(&rawtime);

        strftime(buffer, sizeof(buffer), "%m-%d %H:%M:%S", timeinfo);
        printf("[%d] ", clientSockfd);
        printf("[Time: %s Client: %d Received message: %s]\n", buffer, clientSockfd, message);
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
    {
        char contactName[20];
        strncpy(contactName, message + 1, strlen(message) - 1);
        contactName[strlen(message) - 1] = '\0';
        editContact(contactName, clientSockfd);
        break;
    }
    case '4':
        break;
    case '5':
        // Display all contacts
        displayAllContacts(clientSockfd);
        break;
    case '0':
        return false;
    }
    return true;
}

void addContact(Contact contact, int clientSockfd)
{
    printf("[%d] ", clientSockfd);
    printf("Adding contact: %s %s\n", contact.nom, contact.prenom);
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
    printf("[%d] ", clientSockfd);
    printf("Contact added\n");
    printf("[%d] ", clientSockfd);
    printf("%s#%s#%d#%s#%s#%s#%s\n",
           contact.nom, contact.prenom, contact.GSM, contact.email,
           contact.adr.rue, contact.adr.ville, contact.adr.pays);

    sendMessage(clientSockfd, "1", 2);
}

void searchContact(char *contactName, int clientSockfd)
{
    printf("Searching for contact: %s\n", contactName);
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
    while (fgets(buffer, sizeof(buffer), contactsFile) != NULL)
    {
        if (buffer[0] == '\n')
            continue;

        char *firstName = strtok(buffer, "#");
        char *lastName = strtok(NULL, "#");

        // Construct the full name from the first and last name
        char contactFullName[40];
        strcpy(contactFullName, firstName);
        strcat(contactFullName, " ");
        strcat(contactFullName, lastName);

        if (strcmp(contactFullName, contactName) == 0)
        {
            found = 1;

            // Extract other fields
            char *token = strtok(NULL, "#");
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
            strncpy(contact.prenom, lastName, sizeof(contact.prenom));
        }
    }

    if (found)
    {
        sendMessage(clientSockfd, (const void *)&contact, sizeof(Contact));
        printf("Founded Contact details sent\n");
    }
    else
    {
        printf("Contact %s not found\n", contactName);
        sendMessage(clientSockfd, "0", 2);
    }

    fclose(contactsFile);
}

void updateContact(char *contactName, Contact *newContact, int clientSockfd)
{
    printf("Updating contact: %s\n", contactName);
    FILE *contactsFile = fopen(CONTACTS_FILE, "r");
    if (contactsFile == NULL)
    {
        perror("Error opening contacts.txt");
        sendMessage(clientSockfd, "0", 2);
        printf("Error message sent\n");
        return;
    }

    FILE *tempFile = fopen("temp.txt", "w");
    if (tempFile == NULL)
    {
        perror("Error creating temporary file");
        sendMessage(clientSockfd, "0", 2);
        printf("Error message sent\n");
        fclose(contactsFile);
        return;
    }

    int found = 0;
    Contact contact;
    char buffer[100];
    while (fgets(buffer, sizeof(buffer), contactsFile) != NULL)
    {
        if (buffer[0] == '\n')
        {
            continue;
        }

        char bufferCopy[100];
        strcpy(bufferCopy, buffer);
        char *firstName = strtok(bufferCopy, "#");
        char *lastName = strtok(NULL, "#");

        // Construct the full name from the first and last name
        char contactFullName[40];
        strcpy(contactFullName, firstName);
        strcat(contactFullName, " ");
        strcat(contactFullName, lastName);

        if (strcmp(contactFullName, contactName) == 0)
        {
            found = 1;
            fprintf(tempFile, "%s#%s#%d#%s#%s#%s#%s\n",
                    newContact->nom, newContact->prenom, newContact->GSM, newContact->email,
                    newContact->adr.rue, newContact->adr.ville, newContact->adr.pays);
        }
        else
        {
            fprintf(tempFile, "%s", buffer);
        }
    }
    fclose(contactsFile);
    fclose(tempFile);

    if (rename("temp.txt", CONTACTS_FILE) != 0)
    {
        perror("Error updating contacts file");
        sendMessage(clientSockfd, "0", 2);
        printf("Error message sent\n");
        return;
    }

    if (found)
    {
        sendMessage(clientSockfd, "1", 2);
        printf("Contact updated successfully\n");
    }
    else
    {
        printf("Contact %s not found\n", contactName);
        sendMessage(clientSockfd, "0", 2);
    }
}

void editContact(char *contactName, int clientSockfd)
{
    
    printf("Editing contact: %s\n", contactName);
    searchContact(contactName, clientSockfd);

    char *responce = receiveMessage(clientSockfd);
    Contact newContact;
    memcpy(&newContact, responce + 1, sizeof(Contact));
    // Update contact
    updateContact(contactName, &newContact, clientSockfd);
}

void deleteContact(int clientSockfd)
{
    printf("Deleting contact\n");
}

void displayAllContacts(int clientSockfd)
{
    FILE *contactsFile = fopen(CONTACTS_FILE, "r");
    if (contactsFile == NULL)
    {
        perror("Error opening contacts.txt");
        sendMessage(clientSockfd, "0", 2);
        printf("Error message sent\n");
        return;
    }

    // read and send all contacts separated by |
    char line[100];
    char message[MAX_MESSAGE_SIZE] = "1";

    while (fgets(line, sizeof(line), contactsFile) != NULL)
    {
        // merge all contacts lines in one string separated by |
        strcat(message, line);
        strcat(message, "|");
    }
    // send the message
    sendMessage(clientSockfd, message, strlen(message));
    fclose(contactsFile);
}
