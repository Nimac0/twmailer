#include "helperFunctions.h"

pthread_mutex_t sendMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t blacklistMutex = PTHREAD_MUTEX_INITIALIZER;

std::string processMessage(std::string clientRequest, std::string sender)
{
    size_t pos = 0;
    std::vector <std::string> dataToBeProcessed;
    for(int i = 0; i < 3; i++) { // SEND[0] recipient[1] subject[2]
        if((pos = clientRequest.find('\n')) == std::string::npos) {
            std::cerr << "couldnt parse data";
            return " ";
        }
        dataToBeProcessed.push_back(clientRequest.substr(0, pos));
        clientRequest.erase(0, pos + 1);
    }
    
    std::regex validUser("[a-z0-9]+"); // Double check incase of malicious user skipping client and accessing server directly
    
    if(std::regex_match(dataToBeProcessed[1], validUser) && dataToBeProcessed[1].size() <= 8
    && dataToBeProcessed[2].length() <= 80) {
        return sender + '\n' + dataToBeProcessed[2] + '\n' + clientRequest;
    }
    return " ";
}

bool addMessage(const std::string message, const std::string recipient, const std::string directoryName)
{
    pthread_mutex_lock(&sendMutex); // locks send so two messages cant have same number
    // Check which number index is at
    std::ifstream inFile(directoryName + "/" + recipient + "/index.txt");
    if (!inFile.is_open())
    {
        return false;
    }
    std::string lastEntry;
    std::getline(inFile, lastEntry);

    // Create message.txt file
    fs::path messageFilePath = fs::path(directoryName) / recipient / (lastEntry + ".txt");

    if (createTextFile(messageFilePath, message))
    {  
        // Rewrite index with the newly incremented latest entry
        std::ofstream outIndex(directoryName + "/" + recipient + "/index.txt");
        if (!outIndex.is_open())
        {
            return false;
        }
        outIndex << std::to_string(1 + std::stoi(lastEntry));
        outIndex.close();
        pthread_mutex_unlock(&sendMutex); 
         
        return true;
    }
    pthread_mutex_unlock(&sendMutex); 
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

std::string getRecipientName(std::string message)
{
    std::stringstream ss(message);
    std::string line;
    std::vector<std::string> messageInLines;

    while (std::getline(ss, line, '\n'))
    {
        messageInLines.push_back(line);
    }
    if (messageInLines.size() < 3)
    {
        return " ";
    }
    return messageInLines[1]; // Recipient
}

bool userExists(const std::string user, const std::string directoryName)
{
    fs::path path = directoryName + "/" + user;
    fs::directory_entry uentry{user};
    return is_directory(path);
}

bool createDirectory(const std::string recipientName, const std::string directoryName)
{
    if(!fs::create_directory(directoryName))
    {
       std::cerr << "Couldnt create directory, already exists" << std::endl;
    }
    
    if (userExists(recipientName, directoryName)) {
        return true;
    }

    fs::path path = directoryName + "/" +  recipientName;

    try
    {
        fs::create_directory(path);
        return createTextFile(path / "index.txt", "0");
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error creating directory: " << e.what() << std::endl;
        return false;
    }
    return true;
}

bool createTextFile(fs::path path, const std::string content)
{
    path.replace_extension(".txt");
    std::ofstream textFile(path);

    // Error opening file
    if (!textFile.is_open())
    {
        return false;
    }   
    textFile << content;
    return true;
}

bool createBlacklist()
{
    if (fs::exists("blacklist.txt"))
    {
        return true;
    }
    std::ofstream textFile("blacklist.txt");
    
    if (!textFile.is_open())
    {
        return false;
    }
    return true;
}

bool manageBlacklist(const std::string clientIP) // Mutexes in helper backlistUser and removeFromBlacklist
{
    bool retValue = blacklistUser(clientIP);
    if (retValue == false)
    {
        return false;
    }
    std::cerr << "User Blacklisted\nWait 1 min\n"; // TODO: Remove
    return removeFromBlacklist(clientIP);
}

bool blacklistUser(const std::string clientIP)
{
    // Append clientIP to end of file (https://stackoverflow.com/questions/47014463/ofstream-open-modes-ate-vs-app)
    pthread_mutex_lock(&blacklistMutex);
    std::ofstream oFile("blacklist.txt", std::ios_base::ate|std::ios_base::in);
    if (!oFile.is_open())
    {
        pthread_mutex_unlock(&blacklistMutex);
        return false; 
    }
    oFile << clientIP + "\n";
    oFile.close();
    pthread_mutex_unlock(&blacklistMutex);
    return true;
}

bool userBlacklisted(const std::string clientIP)
{
    // Scan the file for clientIP (https://stackoverflow.com/questions/13996897/is-there-a-way-to-scan-a-txt-file-for-a-word-or-name-in-c)
    typedef std::istream_iterator<std::string> InIt;
    if (std::find(InIt(std::ifstream("blacklist.txt") >> std::skipws), InIt(), clientIP) != InIt())
    {
        return true;
    }
    return false;
}

bool removeFromBlacklist(const std::string clientIP)
{
    pthread_mutex_lock(&blacklistMutex);
    // Read contents from file and remove clientIP
    std::ifstream inFile("blacklist.txt");
    if (!inFile.is_open())
    {
        pthread_mutex_unlock(&blacklistMutex);
        return false; // TODO: Change --> if error opening file, user gets to keep trying...
    }
    std::string contents(std::istreambuf_iterator<char>{inFile}, {});
    inFile.close();
    pthread_mutex_unlock(&blacklistMutex);

    // Find the clientIP
    size_t pos = contents.find(clientIP);
    if (pos == std::string::npos) // If not found (not valid position)
        return false;

    // Wait 1 minute before erasing clientIP
    std::cerr << "Start the wait\n";
    std::this_thread::sleep_for(std::chrono::minutes(1));
    contents.erase(pos, clientIP.length() + 1); // Plus one to delete the new line
    
    // Replace file contents
    pthread_mutex_lock(&blacklistMutex);
    std::ofstream oFile("blacklist.txt");
    if (!oFile.is_open())
    {
        pthread_mutex_unlock(&blacklistMutex);
        return false;
    }
    oFile << contents;
    pthread_mutex_unlock(&blacklistMutex);
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

bool validPort(const std::string port)
{
    std::regex validPort("^((6553[0-5])|(655[0-2][0-9])|(65[0-4][0-9]{2})|(6[0-4][0-9]{3})|([1-5][0-9]{4})|([0-5]{0,5})|([0-9]{1,4}))$"); // Range: 0 to 65535
    return std::regex_match(port, validPort);
}

