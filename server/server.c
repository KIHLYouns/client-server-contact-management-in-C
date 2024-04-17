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
void processClientRequest(int client_sockfd, char *role);
bool authenticateUser(int connfd, char *role);
void addContact(Contact contact);
// ... other contact management functions

int main()
{
    // 1. Create a socket
    int server_sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_sockfd == -1)
    {
        printf("Socket creation failed...\n");
        exit(0);
    }
    else
    {
        printf("Socket successfully created..\n");
    }

    // 2. Prepare the server address structure (sockaddr_in)
    struct sockaddr_in serv_addr;
    serv_addr.sin_family = AF_INET;         // IPv4 address family
    serv_addr.sin_addr.s_addr = INADDR_ANY; // Accept connections on any interface
    serv_addr.sin_port = htons(PORT);       // Convert port number to network byte order

    // 3. Bind the socket to the specified IP address and port
    if (bind(server_sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) != 0)
    {
        printf("Socket bind failed...\n");
        exit(0);
    }
    else
    {
        printf("Socket successfully binded..\n");
    }

    // 4. Enable the socket to listen for connections
    if (listen(server_sockfd, 5) != 0)
    {
        printf("Listen failed...\n");
        exit(0);
    }
    else
    {
        printf("Server listening..\n");
    }

    while (true)
    {
        // Accept a connection
        struct sockaddr_in cli;      // Client address structure
        socklen_t len = sizeof(cli); // Size of the client address structure
        int client_sockfd = accept(server_sockfd, (struct sockaddr *)&cli, &len);

        if (client_sockfd < 0)
        {
            printf("Server accept failed...\n");
            exit(0);
        }
        else
        {
            printf("Server accepted the client...\n");
        }

        char role[10]; // To store the user's role
        if (authenticateUser(client_sockfd, role))
        {
            printf("User authenticated with role: %s\n", role);
            processClientRequest(client_sockfd, role); // Send the role to processClientRequest
        }
        else
        {
            // ... authentication failure ...
        }

        // ... close the client socket ...
    }
}

bool authenticateUser(int connfd, char *role)
{
    // 1. Receive login credentials from the client
    Login login;
    read(connfd, &login, sizeof(Login));

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
            printf("Received: %s %s\n", login.username, login.password); 
            printf("Stored: %s %s %s\n", storedUsername, storedPassword, storedRole);

            // 4. Basic comparison (consider password hashing!)
            if (strcmp(storedUsername, login.username) == 0 &&
                strcmp(storedPassword, login.password) == 0)
            {
                strcpy(role, storedRole); // Copy the role into the output parameter
                fclose(usersFile);
                write(connfd, "1", 1); // Send success indicator to the client
                return true;
            }
        }
    }

    // 5. Authentication failed
    fclose(usersFile);
    write(connfd, "0", 1); // Send failure indicator to the client
    return false;
}

void processClientRequest(int client_sockfd, char *role)
{
    char message[MAX_MESSAGE_SIZE];
    int bytes_received = recv(client_sockfd, message, MAX_MESSAGE_SIZE, 0);

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
            if (recv(client_sockfd, &newContact, sizeof(Contact), 0) > 0)
            {
                addContact(newContact);
                break;
            }
            /*
            case '2': // Search contact
                searchContact(client_sockfd);
                break;
            // ... cases for edit, delete, display ...
            default:
                sendErrorMessage(client_sockfd, "Invalid command");
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
                searchContact(client_sockfd);
                break;
            case '5': // Display all contacts
                displayContacts(client_sockfd);
                break;
            default:
                sendErrorMessage(client_sockfd, "Invalid command");

            }
        }
        else
        {
            // Handle invalid role (unexpected if your setup is correct)
            sendErrorMessage(client_sockfd, "Invalid role");
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

// ... Implementations of processClientRequest, addContact, etc.
