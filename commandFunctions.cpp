#include "commandFunctions.h"
#include "ldapClient.h"

pthread_mutex_t loginMutex = PTHREAD_MUTEX_INITIALIZER;

//LOGIN
// LDAPClient.authentificateUser RETURN CODES:
//  0 --> Success
// -1 --> Error
// -2 --> Falied login 
int handleLogin(std::string message, std::string *username)
{
    LDAPClient ldapClient;
    std::vector<std::string> credentials;
    pthread_mutex_lock(&loginMutex);
    if (!getCredentials(message, credentials))
    {
        return -1;
    }
    int returnCode = ldapClient.authenticateUser(credentials[0], credentials[1]);
    if(returnCode == 0)
    {
        *username = credentials[0];// not sure if threadsafe
    }
    pthread_mutex_unlock(&loginMutex);
    return returnCode;
}
//SEND
bool handleSend(const std::string message, std::string)
{
    if((processMsg(message).compare(" ") == 0)) return 0; // return 0 when error occurs

    if (createDirectory(getRecipientName(message))) {
        addMsg(processMsg(message), getRecipientName(message));
        return 1;
    }
    return 0;
}
//LIST
std::string handleList(const std::string username)
{
    // Count of msgs (0 is no message or user unknown)
    int count = 0;

    fs::path basePath = "spool";
    fs::path userPath = basePath / username;

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
                count++;
                // Open current file
                std::fstream inFile(path);
                if (!inFile.is_open())
                    return "ERR\n";
                // Save subject in subjectList vector
                std::string subject; 
                getline(gotoLine(inFile, 2), subject);
                std::string filename = path.filename();
                std::string noExt = filename.find_last_of(".") == std::string::npos ? filename : filename.substr(0, filename.find_last_of("."));
                subjectList.push_back(noExt + ": " + subject);
            }
        }
        // Turn vector into string
        std::string message = std::to_string(count) + "\n";
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
//READ
std::string handleRead(std::string message, std::string username)// TODO message does not just contain number need to fix
{   
    fs::path filepath = fs::path("spool")/username/(message + ".txt"); //message should contain only the number of message

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
        std::cout << msgPiece << std::endl;
        if(count > 3 && msgPiece != ".")
        {
            requestedMsg += msgPiece + "\n";
        }
    }
    file.close();

    return requestedMsg;
}
//DEL
bool handleDelete(std::string message, std::string username)
{
    fs::path filepath = fs::path("spool")/username/(message + ".txt"); //message should only contain number of message to be deleted
    if(fs::remove(filepath) == 0)
    {
        std::cerr << "file deletion error occured" << std::endl;
        return 0; //return 0 on error
    }
    return 1;
}