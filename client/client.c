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

int connectToServer(const char *serverIP, int serverPort);
int login(int sockfd, char *role);
void displayMenu(const char *role);
int getMenuChoice();
void addContact(int sockfd);
void searchContact(int sockfd);
void editContact(int sockfd);
void deleteContact(int sockfd);
void displayAllContact(int sockfd);

int sendMessage(int sockfd, const void *message, size_t length);
char *receiveMessage(int sockfd);

int main()
{
    char role[10];
    char *serverName = "localhost";
    int serverPort = 8080;

    int sockfd = connectToServer(serverName, serverPort);
    if (sockfd < 0)
    {
        fprintf(stderr, "Connection failed.\n");
        return -1;
    }

    if (!login(sockfd, role))
    {
        fprintf(stderr, "Login failed. Exiting.\n");
        close(sockfd);
        return -1;
    }

    int choice;
    do
    {
        displayMenu(role);
        choice = getMenuChoice(role);

        printf("\n"); // Add newline to improve print output

        if (choice < 0 || choice > 5)
        {
            fprintf(stderr, "Invalid option.\n");
        }
        else
        {
            switch (choice)
            {
            case 1:
                addContact(sockfd);
                break;
            case 2:
                searchContact(sockfd);
                break;
            case 3:
                editContact(sockfd);
                break;
            case 4:
                deleteContact(sockfd);
                break;
            case 5:
                displayAllContact(sockfd);
                break;
            case 0:
                sendMessage(sockfd, "0", 2);
                close(sockfd);
                break;
            }
        }
    } while (choice != 0);

    printf("Exiting...\n");
    return 0;
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

int connectToServer(const char *serverName, int serverPort)
{
    struct hostent *server;

    server = gethostbyname(serverName);
    if (server == NULL) {
        fprintf(stderr,"ERROR, no such host\n");
        return -1;
    }

    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1)
    {
        perror("socket creation failed");
        return -1;
    }

    struct sockaddr_in serverAddress;
    memset(&serverAddress, 0, sizeof(serverAddress));
    serverAddress.sin_family = AF_INET;
    bcopy((char *)server->h_addr, 
         (char *)&serverAddress.sin_addr.s_addr,
         server->h_length);
    serverAddress.sin_port = htons(serverPort);

    if (connect(sockfd, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) != 0)
    {
        perror("Connection failed");
        close(sockfd);
        return -1;
    }

    printf("Connected to server [<->]\n");
    return sockfd;
}

int login(int sockfd, char *role)
{
    Login loginCredentials;

    for (int attempt = 0; attempt < 3; attempt++)
    {
        printf("\nEnter username: ");
        fgets(loginCredentials.username, sizeof(loginCredentials.username), stdin);
        loginCredentials.username[strcspn(loginCredentials.username, "\n")] = 0;
        printf("Enter password: ");
        fgets(loginCredentials.password, sizeof(loginCredentials.password), stdin);
        loginCredentials.password[strcspn(loginCredentials.password, "\n")] = 0;

        if (sendMessage(sockfd, &loginCredentials, sizeof(Login)) != 0)
        {
            fprintf(stderr, "Error sending login credentials.\n");
            continue;
        }
        printf("\nloginCredentials Sent [->]\n");

        char *response = receiveMessage(sockfd);
        printf("[->] loginCredentials Response received\n");

        if (response == NULL)
        {
            fprintf(stderr, "Error receiving authentication response.\n");
            continue;
        }
        // printf("loginCredentials Response received\n");

        if (response[0] == '1')
        {
            printf("\nLogin successful!\n");

            strncpy(role, response + 1, strlen(response) - 1);
            role[strlen(response) - 1] = '\0';

            free(response);
            return 1;
        }

        if (response[0] == '0')
        {
            fprintf(stderr, "\nInvalid username or password...\n");

            char choice[10];
            printf("Try again? (RETRY/EXIT): ");
            fgets(choice, sizeof(choice), stdin);
            choice[strcspn(choice, "\n")] = 0;

            if (strcasecmp(choice, "RETRY") == 0)
            {
                continue;
            }
            else
            {
                printf("Exiting...\n");
                close(sockfd);
                return 0;
            }
        }
    }
    fprintf(stderr, "Too many login attempts !!!\n");
    return 0;
}

void displayMenu(const char *role)
{
    printf("\n------- ROLE: %s -------\n", role);
    printf("\n----- CONTACT MANAGER -----\n");
    if (strcmp(role, "admin") == 0)
    {
        printf("\n1. Add Contact\n");
        printf("2. Search Contact\n");
        printf("3. Edit Contact\n");
        printf("4. Delete Contact\n");
        printf("5. Display All Contacts\n");
    }
    else
    { // Guest role
        printf("\n1. Search Contact\n");
        printf("2. Display All Contacts\n");
    }
    printf("0. Exit\n");
    printf("Enter your choice: ");
}

int getMenuChoice(const char *role)
{

    int choice;
    if (strcmp(role, "admin") == 0)
        while (scanf("%d", &choice) != 1 || choice < 0)
        {
            fprintf(stderr, "Invalid choice. Please enter a valid number: ");
            fflush(stdin);
        }

    else if (strcmp(role, "guest") == 0)
    {
        while (scanf("%d", &choice) != 1 || choice < 0 || choice > 2)
        {
            fprintf(stderr, "Invalid choice. Please enter a valid number: ");
            fflush(stdin);
        }
        if (choice == 1)
        {
            return 2;
        }
        else if (choice == 2)
        {
            return 5;
        }
    }
    return choice;
}

void addContact(int sockfd)
{
    Contact newContact;

    // Prompt user for contact information
    printf("\nEnter contact details: \n");
    printf("Name: ");
    scanf("%s", newContact.nom);
    printf("Surname: ");
    scanf("%s", newContact.prenom);
    printf("GSM: +212");
    scanf("%d", &newContact.GSM);
    printf("Email: ");
    scanf("%s", newContact.email);
    printf("Address: \n");
    printf("   Street: ");
    scanf("%s", newContact.adr.rue);
    printf("   City: ");
    scanf("%s", newContact.adr.ville);
    printf("   Country: ");
    scanf("%s", newContact.adr.pays);

    // Send add contact request
    char message[MAX_MESSAGE_SIZE];
    message[0] = '1'; // Indicates add contact request
    memcpy(message + 1, &newContact, sizeof(Contact));

    if (sendMessage(sockfd, message, sizeof(Contact) + 1) != 0)
    {
        fprintf(stderr, "Error sending add contact request.\n");
    }
    printf("\nnewContact Request sent [->] \n");

    char *response = receiveMessage(sockfd);
    printf("[->] response Message received\n");

    if (response == NULL)
    {
        fprintf(stderr, "Error receiving authentication response.\n");
    }

    else if (strcmp(response, "1") == 0)
    {
        printf("\nContact added successfully.\n");
    }

    else
    {
        fprintf(stderr, "Failed to add contact.\n");
    }

    free(response);
}

void searchContact(int sockfd)
{
    char message[MAX_MESSAGE_SIZE];
    char firstName[20], lastName[20];
    printf("Enter first name: ");
    scanf("%s", firstName);
    printf("Enter last name: ");
    scanf("%s", lastName);

    // Construct the full name
    char fullName[40];
    strcpy(fullName, firstName);
    strcat(fullName, " ");
    strcat(fullName, lastName);

    message[0] = '2';
    memcpy(message + 1, fullName, strlen(fullName) + 1);
    if (sendMessage(sockfd, message, strlen(fullName) + 2) != 0)
    {
        fprintf(stderr, "Error sending search contact request.\n");
    }
    else
    {
        printf("\nsearchContact Request sent [->]\n");
    }

    char *response = receiveMessage(sockfd);
    printf("[->] response Message received\n");
    if (response == NULL)
    {
        fprintf(stderr, "Error receiving authentication response.\n");
    }
    else if (response[0] == '0')
    {
        printf("\nContact not found.\n");
    }
    else
    {
        Contact contact;
        memcpy(&contact, response, sizeof(Contact));
        printf("\nContact found:\n");
        printf("\nName: %s %s\n", contact.nom, contact.prenom);
        printf("GSM: +212%d\n", contact.GSM);
        printf("Email: %s\n", contact.email);
        printf("Address: \n");
        printf("   Street: %s\n", contact.adr.rue);
        printf("   City: %s\n", contact.adr.ville);
        printf("   Country: %s\n", contact.adr.pays);
    }

    free(response);
}

void editContact(int sockfd)
{
    char message[MAX_MESSAGE_SIZE];
    char firstName[20], lastName[20];
    printf("Enter first name: ");
    scanf("%s", firstName);
    printf("Enter last name: ");
    scanf("%s", lastName);

    // Construct the full name
    char fullName[40];
    strcpy(fullName, firstName);
    strcat(fullName, " ");
    strcat(fullName, lastName);

    message[0] = '3';
    memcpy(message + 1, fullName, strlen(fullName) + 1);
    if (sendMessage(sockfd, message, strlen(fullName) + 2) != 0)
    {
        fprintf(stderr, "Error sending edit contact request.\n");
    }
    else
    {
        printf("\neditContact Request sent [->]\n");
    }
    // Wait for the server to confirm that the contact was found
    char *response = receiveMessage(sockfd);
    printf("[->] response Message received\n");
    if (response[0] == '0')
    {
        printf("\nContact not found\n");
        return;
    }
    // Parse the name and surname contact information
    Contact contact;
    memcpy(&contact, response, sizeof(Contact));
    printf("\nContact found:\n");
    printf("\nName: %s %s\n", contact.nom, contact.prenom);
    printf("GSM: +212%d\n", contact.GSM);
    printf("Email: %s\n", contact.email);
    printf("Address: \n");
    printf("   Street: %s\n", contact.adr.rue);
    printf("   City: %s\n", contact.adr.ville);
    printf("   Country: %s\n", contact.adr.pays);
    // Ask the user to enter the new contact information
    Contact newContact;
    printf("Enter newContact information: \n");
    printf("Name: ");
    scanf("%s", newContact.nom);
    printf("Surname: ");
    scanf("%s", newContact.prenom);
    printf("GSM: +212");
    scanf("%d", &newContact.GSM);
    printf("Email: ");
    scanf("%s", newContact.email);
    printf("Address: \n");
    printf("   Street: " );
    scanf("%s", newContact.adr.rue);
    printf("   City: ");
    scanf("%s", newContact.adr.ville);
    printf("   Country: ");
    scanf("%s", newContact.adr.pays);

    // Send the updated contact information
    char buffer[sizeof(Contact) + 1] = "1";
    memcpy(buffer + 1, &newContact, sizeof(Contact));
    sendMessage(sockfd, buffer, sizeof(buffer));
    printf("\nupdateContact Request sent [->]\n");

    // Wait for the server to confirm that the contact was updated
    response = receiveMessage(sockfd);
    printf("[->] response Message received\n");
    if (response[0] == '1')
    {
        printf("\nContact updated successfully\n");
    }
    else
    {
        printf("Error updating contact\n");
    }
}

void deleteContact(int sockfd)
{
    printf("it works");
}

void displayAllContact(int sockfd)
{
    // Send display all contact request
    char message[MAX_MESSAGE_SIZE];
    message[0] = '5';
    if (sendMessage(sockfd, message, 1) != 0)
    {
        fprintf(stderr, "Error sending display all contacts request.\n");
    }
    else
    {
        printf("displayAllContacts Request sent [->]\n");
    }
    // Receive display all contact response
    char *response = receiveMessage(sockfd);
    printf("[->] response Message received\n");
    if (response == NULL)
    {
        fprintf(stderr, "Error receiving display all contacts response.\n");
    }
    else if (response[0] == '0')
    {
        printf("\nNo contacts found.\n");
    }
    else
    {
        printf("\n----- Contacts -----\n\n");
        // Parse and display contacts
        char *token = strtok(response + 1, "|");
        while (token != NULL)
        {
            // Parse token from %s#%s#%d#%s#%s#%s#%s| response format
            Contact contact;
            sscanf(token, "%[^#]#%[^#]#%d#%[^#]#%[^#]#%[^#]#%[^#]",
                contact.nom,
                contact.prenom,
                &contact.GSM,
                contact.email,
                contact.adr.rue,
                contact.adr.ville,
                contact.adr.pays);

            // Display the contact
            printf("Name: %s %s\n", contact.nom, contact.prenom);
            printf("GSM: +212%d\n", contact.GSM);
            printf("Email: %s\n", contact.email);
            printf("Address: \n");
            printf("   Street: %s\n", contact.adr.rue);
            printf("   City: %s\n", contact.adr.ville);
            printf("   Country: %s\n", contact.adr.pays);

            // Get the next token
            token = strtok(NULL, "|");
        }
    }
}
