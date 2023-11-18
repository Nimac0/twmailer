#include "helperFunctions.h"

//-------------------FW DECLARATIONS--------------------------------

int handleLogin(std::string message, std::string *username);
bool handleSend(const std::string message, std::string sender, const std::string directoryName);
std::string handleList(const std::string username, const std::string directoryName);
std::string handleRead(std::string message, const std::string username, const std::string directoryName);
bool handleDelete(std::string message, const std::string username, const std::string directoryName);
//------------------------------------------------------------------
