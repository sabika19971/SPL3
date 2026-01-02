#include <vector>
#include <map>
#include <iostream>
#include <iomanip>
#include "json.hpp"
#include <fstream>
#include "event.h"
#include "ConnectionHandler.h"
#include <thread>
#include <mutex>
class Event;
using json = nlohmann::json;

class KeyBoardInput{
   private: 
          bool isConnected;
          int reciptId;
          int subId;
          const std:: string host; // const host to send to server. sets to "host:stomp.cs.bgu.ac.il"
          const std:: string a_v;
          std::string userName;
          std::mutex& sharedDataMutex;
          std::map<std::string,std::vector<Event>>& eventsReported;
          std:: map<std::string,int> mapForSubscription;
          std::vector<std::string> splitAction;
        
   public:    
        KeyBoardInput(std::map<std::string,std::vector<Event>>& _eventsReported, std::mutex& _sharedDataMutex);
        void sender(ConnectionHandler& ch);
        void splitter (const std::string& input); // split the input from the user.
        ConnectionHandler& setfirstConnection();
        std::string epochToDate(std::time_t epochTime);
        void summery(const std::string channel, const std::string user, const std::string file);
        std::vector<int> dataRep(const std::vector<Event>& userReports); 
        const std::string get_short_desc( std::string description);
        bool connection();


};