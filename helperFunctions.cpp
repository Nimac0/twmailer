#include "helperFunctions.h"

std::string processMsg(std::string clientRequest)
{
    size_t pos = 0;
    std::vector <std::string> dataToBeProcessed;

    for(int i = 0; i < 3; i++) {
        if((pos = clientRequest.find('\n')) == std::string::npos) {
            std::cout << "couldnt parse data";
            return " ";
        }
        dataToBeProcessed.push_back(clientRequest.substr(0, pos));
        clientRequest.erase(0, pos + 1);
    }
    
    std::regex validUser("[a-z0-9]+"); // Double check incase of malicious user skipping client and accessing server directly
    
    if(std::regex_match(dataToBeProcessed[1], validUser) && dataToBeProcessed[1].size() <= 8
    && std::regex_match(dataToBeProcessed[2], validUser) && dataToBeProcessed[2].size() <= 8) {
        return dataToBeProcessed[1] + '\n' + dataToBeProcessed[2] + '\n' + clientRequest;
    }
    return " ";
}

bool addMsg(const std::string message, const std::string user)
{
    // Check which number index is at
    std::ifstream inFile("spool/" + user + "/index.txt");
    if (!inFile.is_open())
        return false;
    std::string lastEntry;
    std::getline(inFile, lastEntry); 

    // Create message.txt file
    fs::path basePath = "spool";
    fs::path messageFilePath = basePath / user / (lastEntry + ".txt");
    if (createTextFile(messageFilePath, message))
    {
        // Rewrite index with the newly incremented latest entry
        std::ofstream outIndex("spool/" + user + "/index.txt");
        if (!outIndex.is_open())
            return false;
        outIndex << std::to_string(1 + std::stoi(lastEntry));
        outIndex.close();

        return true;
    }
    return false;
} 

bool getCredentials(const std::string message, std::vector<std::string>& credentialsVector)
{
    std::stringstream ss(message);
    std::string line;
    std::vector<std::string> messageInLines;

    while (std::getline(ss, line, '\n'))
    {
        messageInLines.push_back(line);
    }
    // Checks that the minimum for the amount of lines in order to get the username is there
    if (messageInLines.size() < 3)
    {
        return false;
    }
    std::regex validUser("[a-z0-9]+");

    if (std::regex_match(messageInLines[1], validUser) && messageInLines[1].size() <= 8)
    {
        credentialsVector.push_back(messageInLines[1]);
        credentialsVector.push_back(messageInLines[2]);
        return true;
    }
    return false;
}

// TODO: Can be eventually deleted
std::string getUsername(std::string message, const std::string command)
{
    std::stringstream ss(message);
    std::string line;
    std::vector<std::string> messageInLines;

    while (std::getline(ss, line, '\n'))
    {
        messageInLines.push_back(line);
    }

    // Checks that the minimum for the amount of lines in order to get the username is there
    if (messageInLines.size() < (command == "LIST" ? 2 : 3))
    {
        std::cerr << "Couldn't parse data" << std::endl;
        return " ";
    }

    std::regex validUser("[a-z0-9]+");

    if (command == "SEND" || command == "LOGIN")
    {
        if (std::regex_match(messageInLines[1], validUser) && messageInLines[1].size() <= 8 &&
            std::regex_match(messageInLines[2], validUser) && messageInLines[2].size() <= 8)
        {
            return messageInLines[2];
        }
    }
    else if (command == "LIST")
    {
        if (std::regex_match(messageInLines[1], validUser) && messageInLines[1].size() <= 8)
        {
            return messageInLines[1];
        }
    }
    else if (command == "READ" || command == "DEL") // When read or delete -> return username and number of file to be interacted with
    {
        if (std::regex_match(messageInLines[1], validUser) && messageInLines[1].size() <= 8)
        {
            return messageInLines[1] + "\n" + messageInLines[2];
        }
    }
    return " ";
}

bool userExists(const std::string user)
{
    fs::path basePath = "spool";
    fs::path userPath = basePath/user;
    fs::directory_entry uentry{user};
    return is_directory(userPath);
}

bool createDirectory(const std::string recipientName)
{
    if(!fs::exists("spool"))
    {
        try
        {
            fs::create_directory("spool");
        }
        catch (const std::exception &e)
        {
            std::cerr << "Error creating directory: " << e.what() << std::endl;
            return false;
        }
    }
    if (!userExists(recipientName))
    {
        fs::path basePath = "spool";
        fs::path userPath = basePath / recipientName;

        try
        {
            fs::create_directory(userPath);
            return createTextFile(userPath / "index.txt", "0");
        }
        catch (const std::exception &e)
        {
            std::cerr << "Error creating directory: " << e.what() << std::endl;
            return false;
        }
    }
    return true;
}

bool createTextFile(fs::path path, const std::string content)
{
    fs::path filePath = "spool"/path;
    filePath.replace_extension(".txt");
    std::ofstream textFile(path);

    // Error opening file
    if (!textFile.is_open())
        return false;
    textFile << content;
    return true;
}

bool createBlacklist()
{
    if (fs::exists("blacklist.txt"))
        return true;
    std::ofstream textFile("blacklist.txt");
    
    if (!textFile.is_open())
        return false;
    return true;
}

bool manageBlacklist(const std::string userIP)
{
    if (!blacklistUser(userIP))
    {
        return false;
    }
    std::cerr << "User Blacklisted\n";
    // TODO: Concurrency --> Shared resource
    std::cerr << "Wait 1 min\n";
    // TODO: Remove blacklist in a thread?
    return removeFromBlacklist(userIP); 
}

bool blacklistUser(const std::string userIP)
{
    // Append userIP to end of file
    // https://stackoverflow.com/questions/47014463/ofstream-open-modes-ate-vs-app
    std::ofstream oFile("blacklist.txt", std::ios_base::ate|std::ios_base::in);
    if (!oFile.is_open())
        return false;
    oFile << userIP + "\n";
    oFile.close();
    return true;
}

bool userBlacklisted(const std::string userIP)
{
    // Scan the file for userIP
    // https://stackoverflow.com/questions/13996897/is-there-a-way-to-scan-a-txt-file-for-a-word-or-name-in-c
    typedef std::istream_iterator<std::string> InIt;
    if (std::find(InIt(std::ifstream("blacklist.txt") >> std::skipws), InIt(), userIP) != InIt())
    {
        return true;
    }
    return false;
}

bool removeFromBlacklist(const std::string userIP)
{
    std::ifstream inFile("blacklist.txt");
    if (!inFile.is_open())
    {
        // TODO: Change --> if error opening file, user gets to keep trying...
        return false;
    }

    // Write contents of file on to a string
    std::string contents(std::istreambuf_iterator<char>{inFile}, {});
    inFile.close();

    size_t pos = contents.find(userIP);
    if (pos == std::string::npos)
        return false;

    // Wait 1 minute
    std::this_thread::sleep_for(std::chrono::minutes(1));
    contents.erase(pos, userIP.length() + 1); // Plus one to delete the new line
    
    // Replace file contents
    std::ofstream oFile("blacklist.txt");
    if (!oFile.is_open())
    {
        return false;
    }
    oFile << contents;
    return true;
}

// https://stackoverflow.com/questions/5207550/in-c-is-there-a-way-to-go-to-a-specific-line-in-a-text-file/5207600
std::fstream& gotoLine(std::fstream& file, unsigned int num)
{
    file.seekg(std::ios::beg);
    for(unsigned int i = 0; i < num - 1; ++i)
    {
        file.ignore(std::numeric_limits<std::streamsize>::max(),'\n');
    }
    return file;
}

