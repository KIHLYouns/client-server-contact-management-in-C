void searchContact(char *contactName, int clientSockfd)
{
    printf("\t[%d] ", clientSockfd);
    printf("Searching for contact: %s\n", contactName);
    FILE *file = fopen("server.log", "a");
    fprintf(file, "\t[%d] ", clientSockfd);
    fprintf(file, "Searching for contact: %s\n", contactName);
    fclose(file);
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

        char contactFullName[40];
        strcpy(contactFullName, firstName);
        strcat(contactFullName, " ");
        strcat(contactFullName, lastName);
        if (strcmp(contactFullName, contactName) == 0)
        {
            found = 1;

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
        printf("\t[%d] ", clientSockfd);
        printf("Contact %s found\n", contactName);
        file = fopen("server.log", "a");
        fprintf(file, "\t[%d] ", clientSockfd);
        fprintf(file, "Contact %s found\n", contactName);
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
    fclose(contactsFile);
}