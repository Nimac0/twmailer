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
//-------------------FW DECLARATIONS--------------------------------
std::string processMsg(std::string clientRequest);
bool addMsg(const std::string bigString, const std::string user);

std::string getUsername(std::string message, const std::string command);
bool userExists(const std::string user);

bool createDirectory(const std::string recipientName);
bool createTextFile(fs::path path, std::string content);

std::fstream& gotoLine(std::fstream& file, unsigned int num);
//------------------------------------------------------------------