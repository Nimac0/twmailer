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
std::string processMsg(std::string clientRequest);
std::string listEmails(const std::string user);
void sendMsg(const std::string message);
std::string getUsername(std::string message);
bool userExists(const std::string user);
bool createDirectory(const std::string recipientName);
bool userExists(const std::string user);
bool addMsg(const std::string bigString, const std::string user);
bool createTextFile(fs::path path, std::string content);
std::fstream& GotoLine(std::fstream& file, unsigned int num);
//createInbox()
//sendmsg()
//readmsg(int index)
//listmsg()
//del(int index)

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

    strcpy(buffer, "Please enter your commands...\r\n");
    if (send(*current_socket, buffer, strlen(buffer), 0) == -1)
    {
        perror("send failed");
        return NULL;
    }

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
            sendMsg(message);
            /*
            if(processMsg(message).compare(" ") == 0)
            {
                perror("error in data sent, couldnt process");
                return NULL;
            }
            */
            if (send(*current_socket, "MESSAGE SENT", 13, 0) == -1) {
                perror("send answer failed");
                return NULL;
            }

            std::cout << "TEST SUCCESSFUL" << std::endl;

        } else if (commandFound(message, "LIST")) {
            
            std::string emailList = listEmails(getUsername(message));

            if (send(*current_socket, emailList.c_str(), emailList.length(), 0) == -1) {
                perror("send answer failed");
                return NULL;
            }
            std::cout << "List works" << std::endl;
        } 
        if (send(*current_socket, "OK", 3, 0) == -1) {
            perror("send answer failed");
            return NULL;
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

std::string processMsg(std::string clientRequest)
{
    size_t pos = 0;
    std::vector <std::string> dataToBeProcessed;

    for(int i = 0; i < 3; i++) {
        if((pos = clientRequest.find('\n')) == std::string::npos) {
            std::cout << "couldnt parse data";
            return " ";
        }
        dataToBeProcessed.push_back(clientRequest.substr(0, pos));
        clientRequest.erase(0, pos + 1);
    }
    
    std::regex validUser("[a-z0-9]+"); //double check incase of malicious user skipping client and accessing server directly
    
    if(std::regex_match(dataToBeProcessed[1], validUser) && dataToBeProcessed[1].size() <= 8
    && std::regex_match(dataToBeProcessed[2], validUser) && dataToBeProcessed[2].size() <= 8) {
        return dataToBeProcessed[1] + '\n' + dataToBeProcessed[2] + '\n' + clientRequest;
    }
    return " ";
}

void sendMsg(const std::string message)
{
    if ((!processMsg(message).compare(" ") == 0))
    {
        std::string uname = getUsername(message);
        if (createDirectory(uname))
            addMsg(processMsg(message), uname);
    }
}

std::string listEmails(const std::string user)
{
    // Count of msgs (0 is no message or user unknown)
    int cnt = 0;
    std::vector<std::string> subjectList;

    // TODO: More elegant solution for paths
    fs::path basePath = "spool";
    fs::path userPath = basePath/user;

    try
    {
        for (auto const& dir_entry : fs::directory_iterator{userPath})
        {
            if (dir_entry.is_regular_file() && !(dir_entry == (userPath/"index.txt")))
            {
                cnt++;

                std::fstream file;
                file.open(dir_entry.path());

                if (!file.is_open())
                    return "ERR";

                std::string subject; 
                std::getline(GotoLine(file, 3), subject);
                subjectList.push_back(subject + " " + dir_entry.path().filename().string());
            }
        }
        // Turn vector into string
        // TODO: Could also be turned into a message stream
        std::string message = std::to_string(cnt);
        for (const auto& subject : subjectList)
        {
            message += subject + "\n";
        }
        return message;
    }
    catch (const std::exception& e)
    {
        return "ERR:" + std::string(e.what());
    }
}

std::string getUsername(std::string message)
{
    size_t pos = 0;
    std::vector <std::string> dataToBeProcessed;

    for(int i = 0; i < 3; i++) {
        if((pos = message.find('\n')) == std::string::npos) {
            std::cout << "couldnt parse data";
            return " ";
        }
        dataToBeProcessed.push_back(message.substr(0, pos));
        message.erase(0, pos + 1);
    }
    
    std::regex validUser("[a-z0-9]+"); //double check incase of malicious user skipping client and accessing server directly
    
    if(std::regex_match(dataToBeProcessed[1], validUser) && dataToBeProcessed[1].size() <= 8
    && std::regex_match(dataToBeProcessed[2], validUser) && dataToBeProcessed[2].size() <= 8) {
        return dataToBeProcessed[2];
    }
    return " ";
}

bool createDirectory(const std::string recipientName)
{
    if(!fs::exists("spool"))
    {
        fs::create_directory("spool");
    }
    if (!userExists(recipientName))
    {
        fs::path basePath = "spool";
        fs::path userPath = basePath / recipientName;

        try
        {
            fs::create_directory(userPath);
            return createTextFile(userPath / "index.txt", "0");
        }
        catch (const std::exception &e)
        {
            std::cerr << "Error creating directory: " << e.what() << std::endl;
            return false;
        }
    }
    return true;
}

bool userExists(const std::string user)
{
    fs::path basePath = "spool";
    fs::path userPath = basePath/user;
    fs::directory_entry uentry{user};
    return is_directory(userPath);
}

bool addMsg(const std::string bigString, const std::string user)
{
    // Check which number index is at
    std::ifstream inFile("spool/" + user + "/index.txt");
    if (!inFile.is_open())
        return false;
    std::string lastEntry;
    std::getline(inFile, lastEntry); 
    fs::path basePath = "spool";
    fs::path messageFilePath = basePath / user / (lastEntry + ".txt");

    if (createTextFile(messageFilePath, bigString))
    {
        // Rewrite index with the newly incremented latest entry
        std::ofstream outIndex("spool/" + user + "/index.txt");
        if (!outIndex.is_open())
            return false;
        outIndex << std::to_string(1 + std::stoi(lastEntry));
        outIndex.close();

        return true;
    }
    return false;
} 

bool createTextFile(fs::path path, std::string content)
{
    fs::path filePath = "spool"/path;
    filePath.replace_extension(".txt");
    std::ofstream textFile(path);
    
    if (!textFile.is_open())
        return false;
    textFile << content;
    return true;
}

// https://stackoverflow.com/questions/5207550/in-c-is-there-a-way-to-go-to-a-specific-line-in-a-text-file/5207600
std::fstream& GotoLine(std::fstream& file, unsigned int num)
{
    file.seekg(std::ios::beg);
    for(unsigned int i = 0; i < num - 1; ++i)
    {
        file.ignore(std::numeric_limits<std::streamsize>::max(),'\n');
    }
    return file;
}

