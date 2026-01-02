#pragma once
#include <iostream>
#include "../include/ConnectionHandler.h"
#include <unordered_map>
#include <iostream>
#include <string>
#include "../include/event.h"

// TODO: implement the STOMP protocol
extern std::atomic<bool> globalVariable; 
class StompProtocol
{
private:
    bool isConnected;
    std::map<std::string,std::vector<Event>>& eventsReported;
    std::mutex& sharedDataMutex;

public:
    StompProtocol(std::map<std::string,std::vector<Event>>& _eventsReported, std::mutex& _sharedDataMutex);
    void reciver(ConnectionHandler& ch);
    std::vector<std::string> splitMessage(const std::string& message);
    std::vector<std::string> parseMessage(const std::string& message);
    std::string getThirdLine(const std::string& input);
    bool connection();    
};
