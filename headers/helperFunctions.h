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
#include <chrono>
#include <future>

namespace fs = std::filesystem;
//-------------------FW DECLARATIONS--------------------------------
std::string processMsg(std::string clientRequest);
bool addMsg(const std::string message, const std::string user);

bool getCredentials(const std::string message, std::vector<std::string>& credentialsVector);
std::string getUsername(std::string message, const std::string command);
bool userExists(const std::string user);

bool createDirectory(const std::string recipientName);
bool createTextFile(fs::path path, const std::string content);
bool createBlacklist();

bool manageBlacklist(const std::string userIP);
bool blacklistUser(const std::string userIP);
bool userBlacklisted(const std::string user);
bool removeFromBlacklist(const std::string userIP);

std::fstream& gotoLine(std::fstream& file, unsigned int num);
//------------------------------------------------------------------