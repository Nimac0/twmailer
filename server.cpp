#include "commandFunctions.h"

namespace fs = std::filesystem;

#define BUF 1024
#define PORT 6543 // TODO: Remove
#define BACKLOG 5 // Count of waiting connections allowed

#define LDAP_LOGIN_FAILED -2
#define LDAP_LOGIN_ERROR -1
#define LDAP_LOGIN_SUCCESS 0

int abortRequested = 0;
int create_socket = -1;
int new_socket = -1;

void *clientCommunication(void *data);
void signalHandler(int sig);
void trimEnd(char* buffer, int* size);
bool commandFound(const std::string message, const std::string command);

int main(int argc, char** argv)
{
    if (argc < 2 || !validPort(std::string(argv[1]))) // argc < 2 for program name and port
    {
        std::cerr << "Error: Invalid Input\nusage: ./server <port>\n";
        return EXIT_FAILURE;
    } 

    //--------------SET UP SOCKET CONNECTION-------------

    socklen_t addrlen;
    struct sockaddr_in address, cliaddress;
    int port = std::stoi(argv[1]);
    int reuseValue = 1;
    pthread_t newThread;

    if (signal(SIGINT, signalHandler) == SIG_ERR) {
        perror("Signal can not be registered");
        return EXIT_FAILURE;
    }
    if ((create_socket = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("Socket error");
        return EXIT_FAILURE;
    }
    if (setsockopt(create_socket, SOL_SOCKET, SO_REUSEADDR, &reuseValue, sizeof(reuseValue)) == -1) {
        perror("Set socket options - reuseAddr");
        return EXIT_FAILURE;
    }
    if (setsockopt(create_socket, SOL_SOCKET, SO_REUSEPORT, &reuseValue, sizeof(reuseValue)) == -1) {
        perror("set socket options - reusePort");
        return EXIT_FAILURE;
    }

    //--------------INIT ADDRESS, BIND, LISTEN-------------
    memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);

    if (bind(create_socket, (struct sockaddr *)&address, sizeof(address)) == -1) {
        perror("bind error");
        return EXIT_FAILURE;
    }
    if (listen(create_socket, BACKLOG) == -1) {
        perror("listen error");
        return EXIT_FAILURE;
    }

    createBlacklist();

    //-----------------ACCEPT NEW CLIENT-----------------
    while(!abortRequested)
    {
        printf("Waiting for connections...\n");
        addrlen = sizeof(struct sockaddr_in);
        if ((new_socket = accept(create_socket, (struct sockaddr *)&cliaddress, &addrlen)) == -1) {
            if (abortRequested) {
                perror("Accept error after aborted");
            } else {
                perror("Accept error"); 
            }
            break;
        }
        printf("Client connected from %s:%d...\n", inet_ntoa(cliaddress.sin_addr), ntohs(cliaddress.sin_port));
        // TODO: Pass on the clientIP -> std::string(inet_ntoa(cliaddress.sin_addr))?
        if((pthread_create(&newThread, NULL, clientCommunication, new int(new_socket))) != 0)// makes new thread, new int -> no race conditions bc of overwriting
        {
            perror("thread error");
        }

        new_socket = -1;
    }
    // Free descriptor
    if (create_socket != -1) {
        if (shutdown(create_socket, SHUT_RDWR) == -1) {
            perror("Shutdown create_socket");
        }
        if (close(create_socket) == -1) {
            perror("Close create_socket");
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
    // std::string message;
    bool loggedIn = false;
    std::string *username = new std::string();
    std::cerr << "new username ptr created" << std::endl;
    pthread_detach(pthread_self());
    std::cerr << "THREAD STARTED, ID: " << pthread_self() << std::endl;

    unsigned int loginAttempts = 0;

    do
    {   
        memset(buffer, 0, sizeof buffer);
        size = recv(*current_socket, buffer, BUF - 1, 0);
        // Recieve until there is nothing more to recieve
        /*
        while (recv(*current_socket, buffer, BUF - 1, 0) > 0) // recv() returns 0 when the socket is shut down normally
        {
            message += std::string(buffer);
        }
        */
    //-----------------ABORT AND ERROR HANDLING-----------------
        if (size == -1) {
            if (abortRequested) {
                perror("Receive error after aborted");
            } else {
                perror("Receive error");
            }
            break;
        }
        if (size == 0) {
            printf("Client closed remote socket\n");
            break;
        }
    //----------------------------------------------------------
        trimEnd(&buffer[0], &size);
        // printf("Message received:\n%s\n", buffer);
        std::string message(buffer);
        if (commandFound(message, "LOGIN") && !loggedIn) {
            // TODO: Disable any recieving until this period is over
            if (userBlacklisted(std::string("debug"))) 
            {
                if (send(*current_socket, "ERR. Please wait one minute before you try again\n", 49, 0) == -1) {
                    std::cerr << "Failed to send answer\n";
                    return NULL;
                }
            }
            std::cout << "USERADRESS: " << username << std::endl;
            int returnCode = handleLogin(message, username);
            std::cout << "USERVALUE: " << *username << std::endl; // TODO: Remove
            std::cerr << "RETURN CODE: " << returnCode << std::endl; // TODO: Remove

            if (returnCode == LDAP_LOGIN_FAILED)
            {
                loginAttempts++;
                std::cerr << "N. of Attempts: " << loginAttempts << "\n"; // TODO: Remove
                if (loginAttempts >= 3) {
                    std::cerr << "In three attempts if\n"; // TODO: Remove
                    if (!manageBlacklist(std::string("debug"))) {
                        std::cerr << "Error removing user from blacklist\n"; // TODO: Remove
                    }
                    loginAttempts = 0;
                }
            }
            else if (returnCode == LDAP_LOGIN_SUCCESS) {
                loggedIn = true;
                if (send(*current_socket, "OK\n", 3, 0) == -1) 
                {
                    std::cerr << "Failed to send answer\n";
                    return NULL;
                }
                continue;
            } 
        } else if (commandFound(message, "SEND") && loggedIn) {
            if (handleSend(message, *username))
            {
                if (send(*current_socket, "OK\n", 3, 0) == -1) {
                    std::cerr << "Failed to send answer\n";
                    return NULL;
                }
                continue;
            } 
        } else if (commandFound(message, "LIST") && loggedIn) {
            std::string emailList = handleList(*username);
            if (send(*current_socket, emailList.c_str(), emailList.length(), 0) == -1) {
                std::cerr << "Failed to send answer\n";
                return NULL;
            }
            continue;
        } else if (commandFound(message, "READ") && loggedIn) {
            std::string returnStr = handleRead(message, *username);
            if (returnStr.compare(" ") != 0) {
                if (send(*current_socket, "OK\n", 3, 0) == -1) {
                    std::cerr << "Failed to send answer\n";
                    return NULL;
                }
                if (send(*current_socket, returnStr.c_str(), returnStr.size() + 1, 0) == -1) {
                    std::cerr << "Failed to send answer\n";
                    return NULL;
                }
                continue;
            }
        } else if (commandFound(message, "DEL") && loggedIn) {
            if (handleDelete(message, *username)) 
            {
                if (send(*current_socket, "OK\n", 3, 0) == -1) {
                    std::cerr << "Failed to send answer\n";
                    return NULL;
                }
                continue;
            }
        }
        if (!commandFound(message, "QUIT")) 
        {
            if (send(*current_socket, "ERR\n", 4, 0) == -1) {
                std::cerr << "Failed to send ERR\n";
                return NULL;
            }
        }
    } while (strcmp(buffer, "QUIT") != 0 && !abortRequested);
    
    delete (username);
    std::cerr << "new username ptr deleted" << std::endl;
// Shut down and close socket connection
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
    delete(current_socket);
    std::cerr << "Current socket ptr deleted" << std::endl;
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
// Compares the first word of the message (command) to any command 
bool commandFound(const std::string message, const std::string command)
{
    // https://www.geeksforgeeks.org/remove-extra-spaces-string/
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