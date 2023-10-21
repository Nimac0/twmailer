#include "helperFunctions.h"

//-------------------FW DECLARATIONS--------------------------------
bool sendMsg(const std::string message);
std::string listEmails(const std::string user);
std::string readMsg(std::string message);
bool delMsg(std::string message);
//------------------------------------------------------------------

//SEND
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
//LIST
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
                getline(gotoLine(inFile, 3), subject);
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
//READ
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