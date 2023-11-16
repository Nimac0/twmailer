#include "clientRequests.h"
#include "mypw.h"

std::string requestLogin()
{
    std::string username;
    std::cout << "enter username: ";
    std::getline(std::cin, username);
    std::cout << "enter password: ";
    std::string password = getpass();

    return std::string("LOGIN") + '\n' + username + '\n' + password;
}

std::string requestList()
{   
    return "LIST";
}

std::string requestRead()
{
    std::string msgNum;
    std::cout << "enter number of message you want to read: ";
    std::getline(std::cin, msgNum);
    
    return std::string("READ") +  '\n' + msgNum + '\n';
}

std::string requestDel()
{
    std::string msgNum;
    std::cout << "enter number of message you want to delete: ";
    std::getline(std::cin, msgNum);
    
    return std::string("DEL") +  '\n' + msgNum;
}

std::string createMsg()
{
    std::string recipient;
    std::string subject;
    std::string message;

    bool valid = false;
    
    do {
        std::cout << "Input Recipient: ";
        std::getline(std::cin, recipient);

        if(!checkUserValidity(recipient)){
            std::cout << "invalid recipient (user cant be longer than 8 chars and must be lowercase and numbers)";
            continue;
        }

        std::cout << "Input Subject(max 80 chars): ";
        std::getline(std::cin, subject);
        if(subject.size() > 80){
            std::cout << "subject too long, must be max 80 chars";
            continue;
        }

        std::cout << "Input Message(end with .\\n): ";

        std::string line;
        while(std::getline(std::cin, line))
        {     
            message.append(line).append("\n");

            if(line == ".") break;
        }
        valid = true;
    }while(!valid);

    return std::string("SEND") + "\n" + recipient + "\n" + subject + "\n" + message + "\n";
}

bool checkUserValidity(std::string username)
{
    std::regex validUser("[a-z0-9]+");
    return (std::regex_match(username, validUser) && username.size() <= 8);
}