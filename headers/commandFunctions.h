#include "helperFunctions.h"

//-------------------FW DECLARATIONS--------------------------------

int handleLogin(std::string message, std::string *username);
bool handleSend(const std::string message, std::string sender);
std::string handleList(const std::string username);
std::string handleRead(std::string message, std::string username);
bool handleDelete(std::string message, std::string username);
//------------------------------------------------------------------
