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
bool sendMsg(const std::string message);
std::string getUsername(std::string message, const std::string command);
bool userExists(const std::string user);
bool createDirectory(const std::string recipientName);
bool addMsg(const std::string bigString, const std::string user);
bool createTextFile(fs::path path, std::string content);
std::fstream& GotoLine(std::fstream& file, unsigned int num);
std::string readMsg(std::string message);
bool delMsg(std::string message);

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

std::string processMsg(std::string clientRequest)
{
    size_t pos = 0;
    std::vector <std::string> dataToBeProcessed;

    for(int i = 0; i < 3; i++) {
        if((pos = clientRequest.find('\n')) == std::string::npos) {
            std::cerr << "couldnt parse data";
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

bool sendMsg(const std::string message)
{
    if((processMsg(message).compare(" ") == 0)) return 0; //return 0 when error occurs

    std::string uname = getUsername(message, "SEND");
    if (createDirectory(uname)) {
        addMsg(processMsg(message), uname);
        return 1;
    }
    return 0;
}

std::string listEmails(const std::string user)
{
    // Count of msgs (0 is no message or user unknown)
    int cnt = 0;

    fs::path basePath = "spool";
    fs::path userPath = basePath / user;

    std::vector<std::string> subjectList;

    try
    {
        std::set<fs::path> sorted_files;

        for (auto const& dir_entry : fs::directory_iterator(userPath)) {
            sorted_files.insert(dir_entry.path());
        }

        for (auto& path : sorted_files )
        {
            if (fs::is_regular_file(path) && !(path == (userPath/"index.txt")))
            {
                cnt++;
                // Open current file
                std::fstream inFile(path);
                if (!inFile.is_open())
                    return "ERR\n";
                // Save subject in subjectList vector
                std::string subject; 
                getline(GotoLine(inFile, 3), subject);
                GotoLine(inFile, 3) >> subject;
                std::string filename = path.filename();
                std::string noExt = filename.find_last_of(".") == std::string::npos ? filename : filename.substr(0, filename.find_last_of("."));
                subjectList.push_back(noExt + ": " + subject);
            }
        }
        // Turn vector into string
        std::string message = std::to_string(cnt) + "\n";
        for (const auto& subject : subjectList)
        {
            message += subject + "\n";
        }
        return message;
    }
    catch (const std::exception& e)
    {
        return "ERR\n";
    }
}

std::string readMsg(std::string message)
{
    std::string data = getUsername(message, "READ");
    
    fs::path filepath = fs::path("spool")/data.substr(0, data.find("\n"))/(data.substr((data.find("\n")) + 1) + ".txt");

    std::ifstream file(filepath);

    if (!file.is_open())
    {
        return " ";
    }

    std::string requestedMsg;
    std::string msgPiece;
    int count = 0;

    while (std::getline(file, msgPiece))
    {
        count++; //only gives out message/ignores previous 3 lines
        if(count > 3 && msgPiece != ".")
        {
            requestedMsg += msgPiece + "\n";
        }
    }
    file.close();

    return requestedMsg;
}

bool delMsg(std::string message)
{
    std::string data = getUsername(message, "READ");
    fs::path filepath = fs::path("spool")/data.substr(0, data.find("\n"))/(data.substr((data.find("\n")) + 1) + ".txt");
    if(fs::remove(filepath) == 0)
    {
        std::cerr << "file deletion error occured" << std::endl;
        return 0; //return 0 on error
    }
    return 1;
}

std::string getUsername(std::string message, const std::string command)
{
    std::stringstream ss(message);
    std::string line;
    std::vector<std::string> dataToBeProcessed;

    while (std::getline(ss, line, '\n'))
    {
        dataToBeProcessed.push_back(line);
    }

    if (dataToBeProcessed.size() < (command == "LIST" ? 2 : 3))
    {
        std::cerr << "couldn't parse data" << std::endl;
        return " ";
    }

    std::regex validUser("[a-z0-9]+");

    if (command == "SEND")
    {
        if (std::regex_match(dataToBeProcessed[1], validUser) && dataToBeProcessed[1].size() <= 8 &&
            std::regex_match(dataToBeProcessed[2], validUser) && dataToBeProcessed[2].size() <= 8)
        {
            return dataToBeProcessed[2];
        }
    }
    else if (command == "LIST")
    {
        if (std::regex_match(dataToBeProcessed[1], validUser) && dataToBeProcessed[1].size() <= 8)
        {
            return dataToBeProcessed[1];
        }
    }
    else if (command == "READ" || command == "DEL") //when read or delete -> return username and number of file to be interacted with
    {
        if (std::regex_match(dataToBeProcessed[1], validUser) && dataToBeProcessed[1].size() <= 8)
        {
            return dataToBeProcessed[1] + "\n" + dataToBeProcessed[2];
        }
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

    // Create message text file
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

