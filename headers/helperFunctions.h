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
#include <thread>
#include <pthread.h>

namespace fs = std::filesystem;

//-------------------FW DECLARATIONS--------------------------------

std::string processMessage(std::string clientRequest, std::string sender);
bool addMessage(const std::string message, const std::string user, const std::string directoryName);

bool getCredentials(const std::string message, std::vector<std::string>& credentialsVector);
std::string getRecipientName(std::string message);
bool userExists(const std::string user, const std::string directoryName);

bool createDirectory(const std::string recipientName, const std::string directoryName);
bool createTextFile(fs::path path, const std::string content);
bool createBlacklist();

bool manageBlacklist(const std::string clientIP);
bool blacklistUser(const std::string clientIP);
bool userBlacklisted(const std::string user);
bool removeFromBlacklist(const std::string clientIP);

std::fstream& gotoLine(std::fstream& file, unsigned int num);
bool validPort(const std::string port);
//------------------------------------------------------------------