#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <netdb.h> 

#define PORT "8080"
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

int connectToServer(); 
int login(int sockfd, char *role);
void displayMenu(const char *role);
int getMenuChoice();
void addContact(int sockfd);

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

int main()
{
    int sockfd = -1;
    char role[10];
    int maxConnectionAttempts = 3; // Customize as needed

    // Connection & Login
    for (int attempt = 0; attempt < maxConnectionAttempts; attempt++)
    {
        sockfd = connectToServer();
        if (sockfd >= 0)
        { // Successful connection
            if (login(sockfd, role))
            {
                break; // Proceed on successful login
            }
            else
            {
                close(sockfd); // Close on failed login attempt
                printf("Login failed. ");
                if (attempt < maxConnectionAttempts - 1)
                {
                    printf("Retrying...\n");
                }
                else
                {
                    printf("Exiting.\n");
                    return -1;
                }
            }
        }
        else
        {
            printf("Connection attempt %d failed.\n", attempt + 1);
        }
    }

    // Main client operation loop (assuming successful login)
    int choice;
    do
    {
        displayMenu(role); // Pass the role to customize the menu
        choice = getMenuChoice();

        switch (choice)
        {
        case 1:
            if (strcmp(role, "admin") == 0)
            {
                addContact(sockfd);
            }
            else
            {
                printf("Unauthorized for this action.\n");
            }
            break;
        // ... other cases ...
        case 0:
            printf("Exiting...\n");
            break;
        default:
            printf("Invalid option.\n");
            break;
        }
    } while (choice != 0);

    close(sockfd);
    return 0;
}

int connectToServer()
{
    int sockfd;
    struct addrinfo hints, *serverInfo, *p;
    int connectionAttempts = 0;
    int maxAttempts = 3; // Adjust the maximum number of attempts as needed

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET; // IPv4
    hints.ai_socktype = SOCK_STREAM;

    while (connectionAttempts < maxAttempts)
    {
        // Resolve the hostname (`getaddrinfo`)
        if (getaddrinfo("localhost", PORT, &hints, &serverInfo) != 0)
        {
            perror("getaddrinfo error");
            return -1;
        }

        // Iterate through the results and try to connect
        for (p = serverInfo; p != NULL; p = p->ai_next)
        {
            if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1)
            {
                perror("socket error");
                continue;
            }

            if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1)
            {
                close(sockfd);
                perror("connect error");
                continue;
            }

            // Successful connection
            freeaddrinfo(serverInfo);
            return sockfd;
        }

        freeaddrinfo(serverInfo);
        connectionAttempts++;

        if (connectionAttempts < maxAttempts)
        {
            printf("Connection failed. Retrying in 5 seconds...\n");
            sleep(5); // Delay before retrying
        }
    }

    printf("Cannot connect to server.\n");
    return -1;
}

int login(int sockfd, char *role)
{
    Login loginCredentials;

    printf("Enter username: ");
    fgets(loginCredentials.username, sizeof(loginCredentials.username), stdin);
    loginCredentials.username[strcspn(loginCredentials.username, "\n")] = 0; // Remove trailing newline

    printf("Enter password: ");
    fgets(loginCredentials.password, sizeof(loginCredentials.password), stdin);
    loginCredentials.password[strcspn(loginCredentials.password, "\n")] = 0;

    // Send login credentials to the server
    if (sendMessage(sockfd, &loginCredentials, sizeof(Login)) != 0)
    {
        printf("Error sending login credentials.\n");
        return -1;
    }

    // Receive authentication response from the server
    char *response = receiveMessage(sockfd);

    if (response == NULL)
    {
        printf("Error receiving authentication response.\n");
        return -1;
    }

    if (response[0] == '1')
    {
        printf("Login successful!\n");

        // Extract the role from the response
        strncpy(role, response + 1, strlen(response) - 1); // Copy the role part of the string
        role[strlen(response) - 1] = '\0';                 // Ensure null termination

        free(response);
        return 1; // Signify successful login
    }

    else
    {
        printf("Invalid username or password.\n");
        free(response);
        return 0; // Signify failed login
    }
}

void displayMenu(const char *role)
{
    printf("Role : %s\n", role);
    printf("\n----- Contact Manager -----\n");
    if (strcmp(role, "admin") == 0)
    {
        printf("1. Add Contact\n");
        printf("2. Search Contact\n");
        printf("3. Edit Contact\n"); // Adapt the options as needed
        printf("4. Delete Contact\n");
        printf("5. Display All Contacts\n");
    }
    else
    { // Guest role
        printf("1. Search Contact\n");
        printf("2. Display All Contacts\n");
    }
    printf("0. Exit\n");
    printf("Enter your choice: ");
}

int getMenuChoice()
{
    int choice;
    while (scanf("%d", &choice) != 1 || choice < 0)
    {
        printf("Invalid choice. Please enter a number: ");
        fflush(stdin); // Clear input buffer
    }
    return choice;
}

void addContact(int sockfd)
{
    Contact newContact;

    printf("Enter contact name: ");
    fgets(newContact.nom, sizeof(newContact.nom), stdin);
    newContact.nom[strcspn(newContact.nom, "\n")] = 0; // Remove trailing newline

    printf("Enter email: ");
    fgets(newContact.email, sizeof(newContact.email), stdin);
    newContact.email[strcspn(newContact.email, "\n")] = 0;

    printf("Enter phone number (GSM): ");
    scanf("%d", &newContact.GSM);

    printf("Enter street address: ");
    fgets(newContact.adr.rue, sizeof(newContact.adr.rue), stdin);
    newContact.adr.rue[strcspn(newContact.adr.rue, "\n")] = 0;

    printf("Enter city: ");
    fgets(newContact.adr.ville, sizeof(newContact.adr.ville), stdin);
    newContact.adr.ville[strcspn(newContact.adr.ville, "\n")] = 0;

    printf("Enter country: ");
    fgets(newContact.adr.pays, sizeof(newContact.adr.pays), stdin);
    newContact.adr.pays[strcspn(newContact.adr.pays, "\n")] = 0;

    // Build the message (assuming protocol format: command;data)
    char message[MAX_MESSAGE_SIZE];
    sprintf(message, "1;%s;%s;%d;%s;%s;%s;%s",
            newContact.nom, newContact.prenom, newContact.GSM, newContact.email,
            newContact.adr.rue, newContact.adr.ville, newContact.adr.pays);

    // Send to  server
    int length = strlen(message);
    if (sendMessage(sockfd, message, length) != 0)
    {
        printf("Error sending contact data.\n");
        return;
    }

    // Receive response from the server
    char *response = receiveMessage(sockfd);
    if (response == NULL)
    {
        printf("Error receiving response from server.\n");
    }
    else if (response[0] == 'E')
    {
        printf("Server Error: %s\n", response + 2);
    }
    else
    {
        // Assume a simple success confirmation is sent
        printf("Contact added successfully!\n");
    }
    free(response);
}
