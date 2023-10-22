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