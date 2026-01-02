
#include "../include/StompProtocol.h"
#include "../include/KeyBoardInput.h"
#include "../include/ConnectionHandler.h"


int main(int argc, char *argv[]) {
    while(true)
    {
        std::map<std::string,std::vector<Event>> eventsReported;
        std::mutex sharedDataMutex;
        KeyBoardInput keyInput(eventsReported, sharedDataMutex);
        ConnectionHandler& ch = keyInput.setfirstConnection();
        StompProtocol protocol(eventsReported, sharedDataMutex);
        std::thread senderThread(&KeyBoardInput::sender,keyInput,std::ref(ch));
        std::thread reciverThread(&StompProtocol::reciver,protocol, std::ref(ch));
        senderThread.join();
        reciverThread.join();
        
        if (!keyInput.connection()|| !protocol.connection()) {
            continue; // Restart the loop
        }   
    }
}



StompProtocol::StompProtocol(std::map<std::string,std::vector<Event>>& _eventsReported, std::mutex& _sharedDataMutex):isConnected(true),eventsReported(_eventsReported), 
                                sharedDataMutex(_sharedDataMutex){};     

bool StompProtocol::connection()
{
    return isConnected;
}

void StompProtocol::reciver(ConnectionHandler& ch)
{
    while(isConnected)
    {    
        std::string answer;
        if (!ch.getLine(answer)) 
        {
            std::cout << "Disconnected. Exiting...\n" << std::endl;
            isConnected = false;
            return;
        }
        
        std::istringstream iss(answer);
        std::string firstWord;
        iss >> firstWord;
        if(firstWord=="MESSAGE")
        {
            std::lock_guard<std::mutex> lock(sharedDataMutex);
            std::vector<std::string> items;
            items = parseMessage(answer);
            Event e = Event(items.at(0), items.at(1));
            eventsReported[e.get_channel_name()].push_back(e); 
        }
        else if(firstWord=="RECEIPT")
        {
            std::cout << getThirdLine(answer) << std::endl;
        }
        else if(firstWord=="ERROR")
        {
            std::cout<< answer <<std::endl;
        }
        else if(firstWord=="CONNECTED")
        {
            std::cout<<"Login successful"<<std::endl;
        }       
    }
}


std::string StompProtocol::getThirdLine(const std::string& input) {
    std::istringstream stream(input); // Use a string stream to process the input line by line
    std::string line;
    int lineNumber = 0;

    // Read each line from the stream
    while (std::getline(stream, line)) {
        ++lineNumber;
        if (lineNumber == 3) { // Return the third line when found
            return line;
        }
    }

    // If the third line doesn't exist, return an empty string or an error message
    return "";
}

std::vector<std::string> StompProtocol::parseMessage(const std::string& message) {
    std::vector<std::string> parsedResults;

    // Extract the part starting from "user:" and ending with the description (and everything after it)
    size_t user_pos = message.find("user:");
    size_t description_pos = message.find("description:");
    
    if (user_pos != std::string::npos && description_pos != std::string::npos) {
        // Extract everything from "user:" to the end of the message (including the description and after it)
        std::string user_to_end = message.substr(user_pos, message.size() - user_pos);
        parsedResults.push_back(user_to_end);
    }

    // Extract the destination value (just the part after "destination:")
    size_t dest_pos = message.find("destination:");
    if (dest_pos != std::string::npos) {
        size_t dest_start = dest_pos + 12; // Length of "destination:"
        size_t dest_end = message.find("\n", dest_start); // Find the newline after the destination
        std::string destination = message.substr(dest_start, dest_end - dest_start); // Get the value after "destination:"
        parsedResults.push_back(destination);
    }

    return parsedResults;
}

std::vector<std::string> StompProtocol::splitMessage(const std::string& message) {
    std::vector<std::string> parts;
    size_t pos = 0;

    // 1. Extract "Message"
    pos = message.find(" ");
    parts.push_back(message.substr(0, pos)); // "Message"
    std::string remaining = message.substr(pos + 1);

    // 2. Extract "subscription :"
    pos = remaining.find(":");
    parts.push_back(remaining.substr(0, pos + 1)); // "subscription :"
    remaining = remaining.substr(pos + 2); // Skip ": "

    // 3. Extract "78"
    pos = remaining.find("\n");
    parts.push_back(remaining.substr(0, pos)); // "78"
    remaining = remaining.substr(pos + 1);

    // 4. Extract "message -id"
    pos = remaining.find(":");
    parts.push_back(remaining.substr(0, pos)); // "message -id"
    remaining = remaining.substr(pos + 1); // Skip ":"

    // 5. Extract "20"
    pos = remaining.find("\n");
    parts.push_back(remaining.substr(0, pos)); // "20"
    remaining = remaining.substr(pos + 1);

    // 6. Extract "destination"
    pos = remaining.find(":");
    parts.push_back(remaining.substr(0, pos)); // "destination"
    remaining = remaining.substr(pos + 1); // Skip ":"

    // 7. Extract "/topic/a"
    pos = remaining.find("\n");
    parts.push_back(remaining.substr(0, pos)); // "/topic/a"
    remaining = remaining.substr(pos + 1);

    // 8. The rest of the message until "^@"
    pos = remaining.find("^@");
    if (pos != std::string::npos) {
        parts.push_back(remaining.substr(0, pos)); // Everything before "^@"
    } else {
        parts.push_back(remaining); // If "^@" is not present, take the rest of the string
    }
    return parts;
}