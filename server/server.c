#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>

#define PORT 8080
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
// ... other contact management functions

bool shouldContinueServingClient(int clientSockfd)
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
            // Consider adding more specific error handling here
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
                else
                {
                    // Handle authentication failure
                }
            } while (shouldContinueServingClient(clientSockfd));

            close(clientSockfd);
        }
    }
}

bool authenticateUser(int clientSockfd, char *role)
{
    // 1. Receive login credentials from the client
    char *response = receiveMessage(clientSockfd);
    Login loginCredentials;
    memcpy(&loginCredentials, response, sizeof(Login));
    free(response);

    // 2. Open the USERS_FILE
    FILE *usersFile = fopen(USERS_FILE, "r");
    if (usersFile == NULL)
    {
        perror("Error opening USERS_FILE");
        return false;
    }

    printf("Trying to open file: %s\n", USERS_FILE);
    // 3. Iterate through the file and compare credentials
    char line[100];
    while (fgets(line, sizeof(line), usersFile) != NULL)
    {
        char storedUsername[20], storedPassword[20], storedRole[10];
        if (sscanf(line, "%s %s %s", storedUsername, storedPassword, storedRole) == 3)
        {
            // 4. comparison
            if (strcmp(storedUsername, loginCredentials.username) == 0 &&
                strcmp(storedPassword, loginCredentials.password) == 0)
            {
                strcpy(role, storedRole); // Copy the role into the output parameter
                fclose(usersFile);

                char response[20];
                sprintf(response, "1%s", role);
                write(clientSockfd, response, strlen(response));
                return true;
            }
        }
    }

    // 5. Authentication failed
    fclose(usersFile);
    write(clientSockfd, "0", 1); // Send failure indicator to the client
    printf("Server sent authentication response.\n");
    return false;
}

void processClientRequest(int clientSockfd, char *role)
{
    char message[MAX_MESSAGE_SIZE];
    int bytes_received = recv(clientSockfd, message, MAX_MESSAGE_SIZE, 0);

    if (bytes_received <= 0)
    {
        // Handle client disconnect or error
        printf("Client disconnected or error.\n");
        return;
    }

    // Process the received command based on role
    if (strcmp(role, "admin") == 0)
    {
        // Handle admin-specific commands
        switch (message[0])
        {         // Assuming the first byte of the message indicates the command
        case '1': // Add contact
        {
            Contact newContact;
            if (recv(clientSockfd, &newContact, sizeof(Contact), 0) > 0)
            {
                addContact(newContact);
                break;
            }
            /*
            case '2': // Search contact
                searchContact(clientSockfd);
                break;
            // ... cases for edit, delete, display ...
            default:
                sendErrorMessage(clientSockfd, "Invalid command");
            */
        }
        }
    }

    else if (strcmp(role, "guest") == 0)
    {
        // Handle guest-specific commands
        switch (message[0])
        {
            /* case '2': // Search contact
                searchContact(clientSockfd);
                break;
            case '5': // Display all contacts
                displayContacts(clientSockfd);
                break;
            default:
                sendErrorMessage(clientSockfd, "Invalid command");

            }
        }
        else
        {
            // Handle invalid role (unexpected if your setup is correct)
            sendErrorMessage(clientSockfd, "Invalid role");
        }
        */
        }
    }
}

void addContact(Contact contact)
{
    // Receive the contact data from the client (already done on the client-side)

    // Open CONTACTS_FILE in append mode
    int fd = open(CONTACTS_FILE, O_WRONLY | O_APPEND | O_CREAT, 0644);
    if (fd < 0)
    {
        perror("Error opening CONTACTS_FILE");
        // Send an error message to the client
        return;
    }

    // Format the contact information
    char contactStr[100];
    sprintf(contactStr, "%s;%s;%d;%s;%s;%s;%s\n",
            contact.nom, contact.prenom, contact.GSM, contact.email,
            contact.adr.rue, contact.adr.ville, contact.adr.pays);

    // Write to the file
    if (write(fd, contactStr, strlen(contactStr)) != strlen(contactStr))
    {
        perror("Error writing to CONTACTS_FILE");
        // Send an error message to the client
    }
    else
    {
        // Send a success message to the client
    }

    close(fd);
}
