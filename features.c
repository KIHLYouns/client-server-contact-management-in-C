client
{
    Contact newContact;

    printf("Enter contact name: ");
    fgets(newContact.nom, sizeof(newContact.nom), stdin);
    newContact.nom[strcspn(newContact.nom, "\n")] = 0;

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
        printf("Contact added successfully!\n");
    }
    free(response);
}

//
{ // Receive the contact data from the client (already done on the client-side)

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