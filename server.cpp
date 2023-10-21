#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <string>
#include <regex>
#include <iostream>
#include <fstream>
#include <vector>
#include <filesystem>
#include <limits>
#include <set>
#include "commandFunctions.h"

namespace fs = std::filesystem;

#define BUF 1024
#define PORT 6543

int abortRequested = 0;
int create_socket = -1;
int new_socket = -1;

void *clientCommunication(void *data);
void signalHandler(int sig);
void trimEnd(char* buffer, int* size);
bool commandFound(const std::string message, const std::string command);

int main(int argc, char** argv)
{
    socklen_t addrlen;
    struct sockaddr_in address, cliaddress;
    int reuseValue = 1; //like a bool

    if (signal(SIGINT, signalHandler) == SIG_ERR)
    {
        perror("signal can not be registered");
        return EXIT_FAILURE;
    }

    if ((create_socket = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        perror("Socket error");
        return EXIT_FAILURE;
    }

    if (setsockopt(create_socket,
                    SOL_SOCKET,
                    SO_REUSEADDR,
                    &reuseValue,
                    sizeof(reuseValue)) == -1)
    {
        perror("set socket options - reuseAddr");
        return EXIT_FAILURE;
    }

    if (setsockopt(create_socket,
                    SOL_SOCKET,
                    SO_REUSEPORT,
                    &reuseValue,
                    sizeof(reuseValue)) == -1)
    {
        perror("set socket options - reusePort");
        return EXIT_FAILURE;
    }

    memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(create_socket, (struct sockaddr *)&address, sizeof(address)) == -1)
    {
        perror("bind error");
        return EXIT_FAILURE;
    }

    if (listen(create_socket, 5) == -1)
    {
        perror("listen error");
        return EXIT_FAILURE;
    }

    while(!abortRequested)
    {
        printf("Waiting for connections...\n");
        addrlen = sizeof(struct sockaddr_in);
        if ((new_socket = accept(create_socket,
                                (struct sockaddr *)&cliaddress,
                                &addrlen)) == -1)
        {
            if (abortRequested)
            {
                perror("accept error after aborted");
            }
            else
            {
                perror("accept error");
            }
            break;
        }
        printf("Client connected from %s:%d...\n",
        inet_ntoa(cliaddress.sin_addr),
        ntohs(cliaddress.sin_port));
        clientCommunication(&new_socket);
        new_socket = -1;
    }

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

void *clientCommunication(void *data)
{
    char buffer[BUF];
    int size;
    int *current_socket = (int *)data;

    do
    {   
        memset(buffer, 0, sizeof buffer);
        size = recv(*current_socket, buffer, BUF - 1, 0);

        if (size == -1)
        {
            if (abortRequested)
            {
                perror("recv error after aborted");
            }
            else
            {
                perror("recv error");
            }
            break;
        }

        if (size == 0)
        {
            printf("Client closed remote socket\n");
            break;
        }
        trimEnd(&buffer[0], &size);
        printf("Message received:\n%s\n", buffer);

        std::string message(buffer);
      
        if (commandFound(message, "SEND")) {
            if(sendMsg(message)){
                if (send(*current_socket, "OK\n", 3, 0) == -1) {
                perror("send answer failed");
                return NULL;
                }
                continue;
            } 
        } else if (commandFound(message, "LIST")) {
            std::string emailList = listEmails(getUsername(message, "LIST"));

            if (send(*current_socket, emailList.c_str(), emailList.length(), 0) == -1) {
                perror("send answer failed");
                return NULL;
            }
            continue;
        } else if (commandFound(message, "READ")) {
            std::string returnStr = readMsg(message);

            if(returnStr.compare(" ") != 0){
                if (send(*current_socket, "OK\n", 3, 0) == -1) {
                perror("send answer failed");
                return NULL;
                }
                if (send(*current_socket, returnStr.c_str(), returnStr.size(), 0) == -1) {
                    perror("send answer failed");
                    return NULL;
                }
                continue;
            }
        } else if (commandFound(message, "DEL")) {
            if(delMsg(message)){
                if (send(*current_socket, "OK\n", 3, 0) == -1) {
                    perror("send answer failed");
                    return NULL;
                }
                continue;
            }
        }
        if(!commandFound(message, "QUIT")){
            if (send(*current_socket, "ERR\n", 4, 0) == -1) {
                perror("send ERR failed");
                return NULL;
            }
        }

    } while (strcmp(buffer, "QUIT") != 0 && !abortRequested);

    if (*current_socket != -1)
    {
        if (shutdown(*current_socket, SHUT_RDWR) == -1)
        {
            perror("shutdown new_socket");
        }
        if (close(*current_socket) == -1)
        {
            perror("close new_socket");
        }
        *current_socket = -1;
    }
    return NULL;
}

void signalHandler(int sig)
{
    if (sig == SIGINT)
    {
        printf("abort Requested... ");
        abortRequested = 1;

        if (new_socket != -1)
        {
            if (shutdown(new_socket, SHUT_RDWR) == -1)
            {
                perror("shutdown new_socket");
            }
            if (close(new_socket) == -1)
            {
                perror("close new_socket");
            }
            new_socket = -1;
        }

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
    }
    else
    {
        exit(sig);
    }
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

bool commandFound(const std::string message, const std::string command)
{
    std::string cmd = "";
    for (auto &ch : message)
    {
        if (ch == '\n' || ch == '\0')
            break;
        else 
            cmd.push_back(ch);
    }
    return (cmd == command);
}