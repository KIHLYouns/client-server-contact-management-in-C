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

bool authenticateUser(int clientSockfd, char *role);
bool processClientRequest(int clientSockfd, char *role);
void *handleClient(void *clientSockfd_void);
void addContact(Contact contact, int clientSockfd);
void searchContact(char *contactName, int clientSockfd);
void editContact(char *contactName, int clientSockfd);
void deleteContact(char *contactName, int clientSockfd);
void updateContact(char *contactName, Contact *newContact, int clientSockfd);
void displayAllContacts(int clientSockfd);

int sendMessage(int sockfd, const void *message, size_t length);
char *receiveMessage(int sockfd);

int main()
{
    FILE *file = fopen("server.log", "a");
    if (file == NULL)
    {
        perror("Error opening server.log");
        return 1;
    }

    int serverSockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSockfd == -1)
    {
        printf("Socket creation failed...\n");
        fprintf(file, "[ERROR] Socket creation failed...\n");
        exit(0);
    }
    else
    {
        printf("Socket successfully created..\n");
        fprintf(file, "Socket successfully created..\n");
    }

    struct sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = htonl(INADDR_ANY);
    serverAddress.sin_port = htons(8080);

    int bindResult;
    do
    {
        bindResult = bind(serverSockfd, (struct sockaddr *)&serverAddress, sizeof(serverAddress));
        if (bindResult != 0)
        {
            perror("Socket bind failed");
            FILE *file = fopen("server.log", "a");
            file = fopen("server.log", "a");
            fprintf(file, "[ERROR] Socket bind failed...\n");
            sleep(2);
            fclose(file);
        }
    } while (bindResult != 0);
    printf("Socket successfully binded..\n");
    fprintf(file, "Socket successfully binded..\n");

    if (listen(serverSockfd, 5) != 0)
    {
        printf("Listen failed...\n");
        fprintf(file, "[ERROR] Listen failed...\n");
        exit(0);
    }
    else
    {
        printf("Server listening..\n");
        fprintf(file, "Server listening..\n");
    }
    fclose(file);

    while (true)
    {
        struct sockaddr_in clientAddress;
        socklen_t len = sizeof(clientAddress);
        int clientSockfd = accept(serverSockfd, (struct sockaddr *)&clientAddress, &len);
        FILE *file = fopen("server.log", "a");
        if (clientSockfd < 0)
        {
            printf("Server accept failed...\n");
            fprintf(file, "[ERROR] Server accept failed...\n");
        }
        else
        {
            printf("Server accepted the client...\n");
            fprintf(file, "Server accepted the client...\n");

            pthread_t client_thread;
            pthread_create(&client_thread, NULL, handleClient, (void *)(long)clientSockfd);
        }
        fclose(file);
    }
}

void *handleClient(void *clientSockfd_void)
{

    int clientSockfd = (int)(long)clientSockfd_void;

    char role[10];
    while (authenticateUser(clientSockfd, role))
    {
        while (processClientRequest(clientSockfd, role))
        {
        }
        FILE *file = fopen("server.log", "a");
        printf("[%d] Client disconnected...\n", clientSockfd);
        fprintf(file, "[%d] Client disconnected...\n", clientSockfd);
        fclose(file);
    }

    close(clientSockfd);

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
                    FILE *file = fopen("server.log", "a");
                    fprintf(file, "[%s = %s] Authenticated by client [%d]\n", storedUsername, role, clientSockfd);
                    fclose(file);
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
        struct tm *timeinfo;
        char buffer[80];
        time(&rawtime);
        timeinfo = localtime(&rawtime);
        strftime(buffer, sizeof(buffer), "%m-%d %H:%M:%S", timeinfo);
        printf("[%d] ", clientSockfd);
        printf("[Time: %s Received message: %s] \n", buffer, message);
        FILE *file = fopen("server.log", "a");
        fprintf(file, "[%d] ", clientSockfd);
        fprintf(file, "[Time: %s Received message: %s ]\n", buffer, message);
        fclose(file);
    }
    switch (message[0])
    {
    case '1':
    {
        Contact newContact;
        memcpy(&newContact, message + 1, sizeof(Contact));
        addContact(newContact, clientSockfd);
        break;
    }
    case '2':
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
    {
        char contactName[20];
        strncpy(contactName, message + 1, strlen(message) - 1);
        contactName[strlen(message) - 1] = '\0';
        deleteContact(contactName, clientSockfd);
    }
    break;
    case '5':

        displayAllContacts(clientSockfd);
        break;
    case '0':
        return false;
    }
    return true;
}

void addContact(Contact contact, int clientSockfd)
{
    printf("\t[%d] ", clientSockfd);
    printf("Adding contact: %s %s\n", contact.nom, contact.prenom);
    FILE *file = fopen("server.log", "a");
    fprintf(file, "\t[%d] ", clientSockfd);
    fprintf(file, "Adding contact: %s %s\n", contact.nom, contact.prenom);
    fclose(file);
    FILE *contactsFile = fopen(CONTACTS_FILE, "a");
    if (contactsFile == NULL)
    {
        perror("Error opening contacts.txt");
        sendMessage(clientSockfd, "0", 2);
        printf("Error message sent\n");
        return;
    }
    char gsm[20];
    sprintf(gsm, "%d", contact.GSM);
    if (strlen(gsm) < 9)
    {
        sendMessage(clientSockfd, "0", 2);
        printf("\t[%d] [ERROR] GSM number is less than 9 digits\n", clientSockfd);
        FILE *file = fopen("server.log", "a");
        fprintf(file, "\t[%d] [ERROR] GSM number is less than 9 digits\n", clientSockfd);
        fclose(file);
        return;
    }

    fprintf(contactsFile, "%s#%s#%d#%s#%s#%s#%s\n",
            contact.nom, contact.prenom, contact.GSM, contact.email,
            contact.adr.rue, contact.adr.ville, contact.adr.pays);
    fclose(contactsFile);
    printf("\t[%d] ", clientSockfd);
    printf("Contact added: ");
    printf("%s#%s#%d#%s#%s#%s#%s\n",
           contact.nom, contact.prenom, contact.GSM, contact.email,
           contact.adr.rue, contact.adr.ville, contact.adr.pays);
    file = fopen("server.log", "a");
    fprintf(file, "\t[%d] ", clientSockfd);
    fprintf(file, "Contact added: ");
    fprintf(file, "%s#%s#%d#%s#%s#%s#%s\n",
            contact.nom, contact.prenom, contact.GSM, contact.email,
            contact.adr.rue, contact.adr.ville, contact.adr.pays);
    fclose(file);
    sendMessage(clientSockfd, "1", 2);
}



void updateContact(char *contactName, Contact *newContact, int clientSockfd)
{
    printf("\t[%d] ", clientSockfd);
    printf("Updating contact: %s\n", contactName);
    FILE *file = fopen("server.log", "a");
    fprintf(file, "\t[%d] ", clientSockfd);
    fprintf(file, "Updating contact: %s\n", contactName);
    fclose(file);
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
        printf("\t[%d] ", clientSockfd);
        printf("Contact updated: ");
        printf("%s#%s#%d#%s#%s#%s#%s\n",
               newContact->nom, newContact->prenom, newContact->GSM, newContact->email,
               newContact->adr.rue, newContact->adr.ville, newContact->adr.pays);
        file = fopen("server.log", "a");
        fprintf(file, "\t[%d] ", clientSockfd);
        fprintf(file, "Contact updated: ");
        fprintf(file, "%s#%s#%d#%s#%s#%s#%s\n",
                newContact->nom, newContact->prenom, newContact->GSM, newContact->email,
                newContact->adr.rue, newContact->adr.ville, newContact->adr.pays);
        fclose(file);
    }
    else
    {
        printf("\t[%d] ", clientSockfd);
        printf("Contact %s not found\n", contactName);
        sendMessage(clientSockfd, "0", 2);
        file = fopen("server.log", "a");
        fprintf(file, "\t[%d] ", clientSockfd);
        fprintf(file, "Contact %s not found\n", contactName);
        fclose(file);
    }
}

void editContact(char *contactName, int clientSockfd)
{
    printf("\t[%d] ", clientSockfd);
    printf("Editing contact: %s\n", contactName);
    FILE *file = fopen("server.log", "a");
    fprintf(file, "\t[%d] ", clientSockfd);
    fprintf(file, "Editing contact: %s\n", contactName);
    fclose(file);
    searchContact(contactName, clientSockfd);
    char *responce = receiveMessage(clientSockfd);
    Contact newContact;
    memcpy(&newContact, responce + 1, sizeof(Contact));

    updateContact(contactName, &newContact, clientSockfd);
}

void deleteContact(char *contactName, int clientSockfd)
{
    printf("\t[%d] ", clientSockfd);
    printf("Deleting contact : %s\n", contactName);
    FILE *file = fopen("server.log", "a");
    fprintf(file, "\t[%d] ", clientSockfd);
    fprintf(file, "Deleting contact : %s\n", contactName);
    fclose(file);
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

        char contactFullName[40];
        strcpy(contactFullName, firstName);
        strcat(contactFullName, " ");
        strcat(contactFullName, lastName);
        if (strcmp(contactFullName, contactName) != 0)
        {
            fprintf(tempFile, "%s", buffer);
        }
        else
        {
            found = 1;
        }
    }
    fclose(contactsFile);
    fclose(tempFile);
    if (found)
    {
        if (rename("temp.txt", CONTACTS_FILE) != 0)
        {
            perror("Error renaming temporary file");
            sendMessage(clientSockfd, "0", 2);
            printf("Error message sent\n");
        }
        else
        {
            sendMessage(clientSockfd, "1", 2);
            printf("\t[%d] ", clientSockfd);
            printf("Contact %s deleted\n", contactName);
            file = fopen("server.log", "a");
            fprintf(file, "\t[%d] ", clientSockfd);
            fprintf(file, "Contact %s deleted\n", contactName);
            fclose(file);
        }
    }
    else
    {
        printf("\t[%d] ", clientSockfd);
        printf("Contact %s not found\n", contactName);
        sendMessage(clientSockfd, "0", 2);
        file = fopen("server.log", "a");
        fprintf(file, "\t[%d] ", clientSockfd);
        fprintf(file, "Contact %s not found\n", contactName);
        fclose(file);
    }
}

void displayAllContacts(int clientSockfd)
{
    printf("\t[%d] ", clientSockfd);
    printf("Displaying all contacts\n");
    FILE *file = fopen("server.log", "a");
    fprintf(file, "\t[%d] ", clientSockfd);
    fprintf(file, "Displaying all contacts\n");
    fclose(file);
    FILE *contactsFile = fopen(CONTACTS_FILE, "r");
    if (contactsFile == NULL)
    {
        perror("Error opening contacts.txt");
        sendMessage(clientSockfd, "0", 2);
        printf("Error message sent\n");
        return;
    }

    char line[100];
    char message[MAX_MESSAGE_SIZE] = "1";
    while (fgets(line, sizeof(line), contactsFile) != NULL)
    {

        strcat(message, line);
        strcat(message, "|");
    }

    sendMessage(clientSockfd, message, strlen(message));
    fclose(contactsFile);
}
