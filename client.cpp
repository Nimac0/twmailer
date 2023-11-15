#include "clientRequests.h"

#define BUF 1024
#define PORT 6543

void printActionsMenu();
void trimEnd(char* buffer, int* size);

int main(int argc, char** argv)
{
    int create_socket;
    char buffer[BUF];
    struct sockaddr_in address;
    // int size;
    cmd cmd = DEFAULT;

//-----------------------CONNECT TO SERVER-----------------------
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
//--------------------------------------------------------------
    do
    {
        printActionsMenu();
        memset(buffer, 0, sizeof buffer);
        printf(">> ");
        if (fgets(buffer, BUF, stdin) != NULL)
        {
            int size = strlen(buffer);
            trimEnd(&buffer[0], &size);
            // checks input request#
            if (!strcmp(buffer, "LOGIN"))
            {
                cmd = LOGIN;
                strcpy(buffer, requestLogin().data());
            } else if(!strcmp(buffer, "SEND")){
                cmd = SEND;
                strcpy(buffer, createMsg().data());
            } else if(!strcmp(buffer, "LIST")) {
                cmd = LIST;
                strcpy(buffer, requestList().data());
            } else if(!strcmp(buffer, "READ")) {
                cmd = READ;
                strcpy(buffer, requestRead().data());
            } else if(!strcmp(buffer, "DEL")) {
                cmd = DEL;
                strcpy(buffer, requestDel().data());
            } else if(!strcmp(buffer, "QUIT")) {
                cmd = QUIT;
            }
            
            // Check size and clean up request before sending
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
                perror("Error receiving");
                break;
            }
            else if (size == 0)
            {
                printf("Server closed remote socket\n");
                break;
            }
            
            buffer[size] = '\0';
            printf("<< %s", buffer);

            if((strcmp("ERR\n", buffer)) == 0) {
                continue;
            }

            if(cmd == LOGIN) {
                if((strcmp("OK\n", buffer)) != 0)
                {
                    // TODO: Try to log in again
                    fprintf(stderr, "<< Server error occured, abort\n");
                    break;
                }
                // 
            }
            else if(cmd == SEND || cmd == DEL) {
                if((strcmp("OK\n", buffer)) != 0)
                {
                    fprintf(stderr, "<< Server error occured, abort\n");
                    break;
                }
            } else if(cmd == READ) {
                size_t pos;
                std::string response = std::string(buffer).substr(0, (pos = std::string(buffer).find("\n")) == std::string::npos ? 0 : pos + 1);
                if((strcmp("OK\n", response.data())) != 0)
                {
                    fprintf(stderr, "<< Server error occured, abort\n");
                    break;
                }
            }
        }
    } while (cmd != QUIT);

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

void printActionsMenu()
{
    std::cout << "Choose an action to perform:\n |SEND|LIST|READ|DEL|QUIT|" << std::endl;
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