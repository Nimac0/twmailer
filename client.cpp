#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <string>
#include <iostream>
#include <regex>

#define BUF 1024
#define PORT 6543

// void printActionsMenu();
void trimEnd(char* buffer, int* size);
std::string createMsg();
bool checkUserValidity(std::string username);
std::string requestList();
std::string requestRead();
std::string requestDel();

int main(int argc, char** argv)
{
    int create_socket;
    char buffer[BUF];
    struct sockaddr_in address;
    int size;
    int isQuit;

    if ((create_socket = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        perror("Socket error");
        return EXIT_FAILURE;
    }

    memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_port = htons(PORT);

    if (argc < 2)
    {
        inet_aton("127.0.0.1", &address.sin_addr);
    }
    else
    {
        inet_aton(argv[1], &address.sin_addr);
    }

    if (connect(create_socket,
                (struct sockaddr *)&address,
                sizeof(address)) == -1)
    {
        perror("Connect error - no server available");
        return EXIT_FAILURE;
    }

    printf("Connection with server (%s) established\n",
            inet_ntoa(address.sin_addr));

    size = recv(create_socket, buffer, BUF - 1, 0);
    if (size == -1)
    {
        perror("recv error");
    }
    else if (size == 0)
    {
        printf("Server closed remote socket\n");
    }
    else
    {
        buffer[size] = '\0';
        printf("%s", buffer); 
    }

    do
    {
        // printActionsMenu();
        memset(buffer, 0, sizeof buffer);
        printf(">> ");
        if (fgets(buffer, BUF, stdin) != NULL)
        {
            int size = strlen(buffer);
            trimEnd(&buffer[0], &size);
            
            if(!strcmp(buffer, "SEND")){
                strcpy(buffer, createMsg().data());
            }

            if(!strcmp(buffer, "LIST")){
                strcpy(buffer, requestList().data());
            }

            if(!strcmp(buffer, "READ")){
                strcpy(buffer, requestRead().data());
            }

            if(!strcmp(buffer, "DEL")){
                strcpy(buffer, requestDel().data());
            }

            isQuit = strcmp(buffer, "QUIT") == 0;
            
            size = strlen(buffer);
            trimEnd(&buffer[0], &size);
            if ((send(create_socket, buffer, size, 0)) == -1) 
            {
                perror("send error");
                break;
            }

            size = recv(create_socket, buffer, BUF - 1, 0);
            if (size == -1)
            {
                perror("recv error");
                break;
            }
            else if (size == 0)
            {
                printf("Server closed remote socket\n");
                break;
            }
            else
            {
                buffer[size] = '\0';
                printf("<< %s\n", buffer);
                if ((strcmp("MESSAGE SENT", buffer)) != 0 && (strcmp("OK", buffer)) != 0 && (strcmp("LISTING ALL RECIEVED EMAILS", buffer)) != 0)
                {
                    fprintf(stderr, "<< Server error occured, abort\n");
                    break;
                }
            }
        }
    } while (!isQuit);

    if (create_socket != -1)
    {
        if (shutdown(create_socket, SHUT_RDWR) == -1)
        {
            perror("shutdown create_socket"); 
        }
        if (close(create_socket) == -1)
        {
            perror("close create_socket");
        }
        
        create_socket = -1;
    }

    return EXIT_SUCCESS;
}

/*
void printActionsMenu()
{
    printf("Possible actions:\nSEND\nLIST\nQUIT\n");
}
*/
std::string requestList()
{
    std::string username;
    std::cout << "enter username to see a list of mails: ";
    std::getline(std::cin, username);
    
    return std::string("LIST") + '\n' + username;
}

std::string requestRead()
{
    std::string username;
    std::cout << "enter username: ";
    std::getline(std::cin, username);
    std::string msgNum;
    std::cout << "enter number of message you want to read: ";
    std::getline(std::cin, msgNum);
    
    return std::string("READ") + '\n' + username + '\n' + msgNum;
}

std::string requestDel()
{
    std::string username;
    std::cout << "enter username: ";
    std::getline(std::cin, username);
    std::string msgNum;
    std::cout << "enter number of message you want to delete: ";
    std::getline(std::cin, msgNum);
    
    return std::string("DEL") + '\n' + username + '\n' + msgNum;
}

std::string createMsg()
{
    std::string sender;
    std::string recipient;
    std::string subject;
    std::string message;

    bool valid = false;
    
    do{   
        std::cout << "Input Sender: ";
        std::getline(std::cin, sender);
        std::cout << "Input Recipient: ";
        std::getline(std::cin, recipient);

        if(!checkUserValidity(sender) || !checkUserValidity(recipient)){
            std::cout << "invalid sender or recipient (user cant be longer than 8 chars and must be lowercase and numbers)";
            continue;
        }

        std::cout << "Input Subject(max 80 chars): ";
        std::getline(std::cin, subject);
        if(subject.size() > 80){
            std::cout << "subject too long, must be max 80 chars";
            continue;
        }

        std::cout << "Input Message(end with .\\n): ";
        while(std::getline(std::cin, message))
        {     
            if(message == "."){
                message + "\n";
                break;
            }
        }
        valid = true;
    }while(!valid);

    return std::string("SEND") + "\n" + sender + "\n" + recipient + "\n" + subject + "\n" + message + "\n";
}

bool checkUserValidity(std::string username)
{
    std::regex validUser("[a-z0-9]+");
    return (std::regex_match(username, validUser) && username.size() <= 8);
}

void trimEnd(char* buffer, int* size)
{
    if (buffer[*size - 2] == '\r' && buffer[*size - 1] == '\n')
    {
        *size -= 2;
        buffer[*size] = 0;
    }
    else if (buffer[*size - 1] == '\n')
    {
        --*size;
        buffer[*size] = 0;
    }
}