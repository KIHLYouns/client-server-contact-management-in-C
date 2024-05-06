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
#define MAX_MESSAGE_SIZE 1024
#define SEPARATOR "|"

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
void editContact(Contact editedContact);
void displayAllContact(int socket);

int sendMessage(int sockfd, const void *message, size_t length);
char *receiveMessage(int sockfd);
bool shouldContinue(int clientSockfd);

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
            do
            {
                char role[10];
                if (authenticateUser(clientSockfd, role))
                {
                    printf("User authenticated with role: %s\n", role);
                    while (true)
                    {
                        if (!processClientRequest(clientSockfd, role))
                        {
                            // If processClientRequest returns false, the client has finished sending requests
                            break;
                        }
                    }
                }
            } while (shouldContinue(clientSockfd));
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

bool shouldContinue(int clientSockfd)
{
    char *response = receiveMessage(clientSockfd);

    return strcmp(response, "RETRY") == 0;
}

bool processClientRequest(int clientSockfd, char *role)
{
    char *message = receiveMessage(clientSockfd);
    printf("choice Message received \n");
    if(strcmp(role,"admin") == 0 ){
    switch (message[0])
    {
    case '1': // Add contact
    {
        Contact newContact;
        memcpy(&newContact, message + 1, sizeof(Contact));
        addContact(newContact, clientSockfd);
        return true;
    }
    case '2':

        return true;
    case '3': 

        return true;
    case '4': 

        return true;
    case '5': 
        displayAllContact(clientSockfd);
        return true;    


    }
    return false;
    }
    else{
         switch (message[0]){
          case '1': 
          
          return true;
          case '2': 
          displayAllContact(clientSockfd);
          return true;

         }
          return false;
    }
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
    fprintf(contactsFile, "%s,%s,%d,%s,%s,%s,%s\n",
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

void displayAllContact(int socket)
{
   char line[MAX_MESSAGE_SIZE];
FILE*contactsFile = fopen(CONTACTS_FILE,"r");
 if (contactsFile == NULL)
 {
perror("Error opening contacts.txt");
 return;
  }
  
    char allLines[MAX_MESSAGE_SIZE * 10]; 
    allLines[0] = '\0'; 

    // Read lines from contacts file and append to the string
    while (fgets(line, sizeof(line), contactsFile) != NULL) {
        strcat(allLines, line);
        strcat(allLines, SEPARATOR); // Use the separator character directly
    }

    // Remove the last separator (since it's not needed at the end)
    allLines[strlen(allLines) - 1] = '\0'; // Remove the last character

    // Send the entire string to the client
    sendMessage(socket, allLines, strlen(allLines) + 1); // Include null terminator

    
    fclose(contactsFile);
    printf("it works\n");
}
