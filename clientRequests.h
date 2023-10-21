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

enum cmd
{
    DEFAULT = 0,
    SEND,
    LIST,
    READ,
    DEL,
    QUIT
};
//-----------------FW DECS---------------------
std::string createMsg();
std::string requestList();
std::string requestRead();
std::string requestDel();
bool checkUserValidity(std::string username);
//---------------------------------------------

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
    
    return std::string("READ") + '\n' + username + '\n' + msgNum + '\n';
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

        std::string line;
        while(std::getline(std::cin, line))
        {     
            message.append(line).append("\n");

            if(line == ".") break;
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