#include "helperFunctions.h"
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

std::string processMsg(std::string clientRequest)
{
    size_t pos = 0;
    std::vector <std::string> dataToBeProcessed;

    for(int i = 0; i < 3; i++) {
        if((pos = clientRequest.find('\n')) == std::string::npos) {
            std::cerr << "couldnt parse data";
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
    pthread_mutex_lock(&mutex);
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

        pthread_mutex_unlock(&mutex); 
         
        return true;
    }

    pthread_mutex_unlock(&mutex); 
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
        fs::create_directory("spool");
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

bool createTextFile(fs::path path, std::string content)
{
    fs::path filePath = "spool"/path;
    filePath.replace_extension(".txt");
    std::ofstream textFile(path);
    
    if (!textFile.is_open())
        return false;
    textFile << content;
    return true;
}

void blacklistUser()
{


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

