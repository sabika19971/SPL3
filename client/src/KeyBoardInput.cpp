#include "../include/KeyBoardInput.h"

KeyBoardInput::KeyBoardInput(std::map<std::string,std::vector<Event>>& _eventsReported, std::mutex& _sharedDataMutex): isConnected(false), reciptId(0),subId(0),
                                 host("host:stomp.cs.bgu.ac.il"), a_v("accept-version:1.2"), userName(""),
                                 sharedDataMutex(_sharedDataMutex),eventsReported(_eventsReported), mapForSubscription(), splitAction(){};


bool KeyBoardInput::connection()
{
    return isConnected;
}

const std::string KeyBoardInput::get_short_desc(std::string description)
{
    std::ostringstream etcDescription;
    // Check if the description is longer than 27 characters
    if (description.length() > 27) 
    {
        // Extract the first 27 characters and append "..."
        etcDescription << description.substr(0, 27) << "...";
        return etcDescription.str(); // Convert ostringstream to string and return
    }
    // If the description is 27 characters or less, return it as is
    return description;
}

std::vector<int> KeyBoardInput::dataRep(const std::vector<Event>& userReports) // NEW FUNCTION
{
    // Initialize counters
    int activeReports = 0;
    int forcesArrivalAtScene = 0;

    // Iterate through the list of events
    for (const auto& event : userReports) {
        // Access the general information map
        const std::map<std::string, std::string>& generalInfo = event.get_general_information();
        for (const auto& pair : generalInfo) {
            if (pair.first == "active" && pair.second == " true") {
                activeReports++;      
            }
            if (pair.first == "forces_arrival_at_scene" && pair.second == " true") {
                forcesArrivalAtScene++;
            }
        }
    }

    // Create and return the result vector
    return {activeReports, forcesArrivalAtScene};
}

ConnectionHandler& KeyBoardInput::setfirstConnection()
{
    while(1)
    {
        std::cout<<"please enter host and port to connect"<<std::endl;
        std::string userInput;
        std::getline(std::cin, userInput);
        splitter(userInput); // holds result at splitAction
        if(splitAction.size()<4)
        {
            std::cout<<"ilegal input."<<std::endl;
            splitAction.clear();
            continue;
        }
        std::string frame;
        if(!isConnected&&splitAction.at(0) != "login") // check if the client tries to send actions request from server befor he sets the connection to server. 
        {
            std::cout<<"You need to Connect the server first."<<std::endl;
            splitAction.clear();
            continue;
        } 
        ConnectionHandler* ch = new ConnectionHandler(splitAction.at(1),std::stoi(splitAction.at(2)));
        if (!ch->connect()) 
        {
            std::cerr << "Cannot connect to " << splitAction.at(1) << ":" << splitAction.at(2) << std::endl;
            splitAction.clear();
            delete ch;
            continue;
        }
        // the connection secceed updat status.
        isConnected=true; 
        // creates frame for send. 
        frame = "CONNECT\n"
                +host+"\n"+
                "login:" + splitAction.at(3) + "\n"+a_v+"\n"+
                "passcode:"+ splitAction.at(4)+"\n"
                "\n";
        if (!ch->sendFrameAscii(frame,'\0'))
        {
            std::cout << "Disconnected. Exiting...\n" << std::endl;
            splitAction.clear();
            delete ch;
            isConnected = false;
            continue;
        }
        userName = splitAction.at(3);
        splitAction.clear();
        return *ch;
    }      
}

// @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

void KeyBoardInput::sender(ConnectionHandler& ch)
{    
    while(isConnected)
    {
        std::string userInput;
        std::getline(std::cin, userInput);
        splitter(userInput);
        std::string frame;
        
        if(splitAction.at(0) == "login") // CONNECT
        {
            if(isConnected)
            {
                std::cout <<"The client is already logged in, log out before trying again" <<std::endl;
                continue;
            }
        }
        else if(splitAction.at(0) == "join") // SUBSCRIBE
        {
            reciptId++;
            if (mapForSubscription[splitAction.at(1)] != 0)
            {
                // ERROR - already signed to the channel
                std::cout << "already joined channel " << splitAction.at(1) << std::endl;
            }
            else
            {
                subId++;
                mapForSubscription[splitAction.at(1)] = subId;

                std::string strid = std::to_string(subId);
                std::string recipt = std::to_string(reciptId);
                frame = "SUBSCRIBE\n"
                "destination:/" + splitAction.at(1) + "\n" + // added "/" after destination: 
                "id:" + strid + "\n" +
                "receipt:" + recipt + "\n";
                if (!ch.sendFrameAscii(frame,'\0'))
                {
                    std::cout << "Disconnected. Exiting...\n" << std::endl;
                    splitAction.clear();
                    isConnected = false;
                }
            }
        }
        else if(splitAction.at(0) == "exit") // UNSUBSCRIBE
        {
            int id = mapForSubscription[splitAction.at(1)];
            mapForSubscription.erase(splitAction.at(1));
            std::string strid = std::to_string(id);
            std::string recip = std::to_string(reciptId++);
            frame = "UNSUBSCRIBE\n"
                    "id:"+strid+"\n"
                    "receipt:" +recip+"\n"
                    "\n";  
            if (!ch.sendFrameAscii(frame,'\0'))
            {
                std::cout << "Disconnected. Exiting...\n" << std::endl;
                splitAction.clear();
                isConnected = false;
                
            }
        }
        else if(splitAction.at(0) == "report") // SEND
        {
            names_and_events events_and_names = parseEventsFile(splitAction.at(1));
            for(const auto& event: events_and_names.events)
            {
                reciptId++;
                std:: string recip = std::to_string(reciptId);
                std::ostringstream stream; 
                stream<<"SEND\n"<<
                "receipt:" <<recip<<"\n"<<
                "destination:/"<<event.get_channel_name()<<"\n\n"<< 
                "user:"<<userName<<"\n"<<
                "city:"<<event.get_city()<<"\n"<<
                "event name: "<<event.get_name()<<"\n"<<
                "date time: "<<event.get_date_time()<<"\n"<<
                "general information: "<<"\n";
                for (const auto& pair : event.get_general_information())
                {
                    stream << pair.first << ": " << pair.second <<"\n"; 
                }
                stream<<"description: \n"<<event.get_description()<<"\n"; 
                frame = stream.str();
                if (!ch.sendFrameAscii(frame,'\0'))
                {
                    std::cout << "Disconnected. Exiting...\n" << std::endl;
                    isConnected = false;
                    splitAction.clear();
                    return;
                }     
            }
        }      
        else if(splitAction.at(0) == "summary") // NOT A FRAME
        {
            summery("/"+splitAction.at(1),splitAction.at(2),splitAction.at(3)); // "/" ADDED HERE
            continue;
        }
        else if(splitAction.at(0) == "logout") // DISCONNECT
        {
            reciptId++;
            std:: string recip = std::to_string(reciptId);
            frame = "DISCONNECT\nreceipt:" + recip +"\n\n"; // ADDED RECEIPTID AND NEEDED /n
            if (!ch.sendFrameAscii(frame,'\0'))
            {              
                std::cout << "Disconnected. Exiting...\n" << std::endl;
                isConnected = false;   
            }
            isConnected = false; 
        }
        splitAction.clear();
    }
}

// @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

void KeyBoardInput::summery(const std::string _channel, const std::string _user, const std::string _file) 
{
    std::lock_guard<std::mutex> lock(sharedDataMutex);
    std::vector<Event> userReports;

    // Ensure the channel exists and there are events to process
    if (eventsReported.find(_channel) == eventsReported.end() || eventsReported[_channel].empty()) {
        std::cerr << "No events found for the given channel: " << _channel << std::endl;
        return;
    }

    // Filter reports by user
    for (size_t i = 0; i < eventsReported[_channel].size(); i++) {
        if (eventsReported[_channel].at(i).getEventOwnerUser() == _user) {
            userReports.push_back(eventsReported[_channel].at(i));
        }
    }
    
    // Sort reports by date and name
    std::sort(userReports.begin(), userReports.end(), [](const Event& a, const Event& b) {
        if (a.get_date_time() == b.get_date_time()) {
            return a.get_name() < b.get_name();
        }
        return a.get_date_time() < b.get_date_time(); // Compare by date
    });

    // Open the file to write the output
    std::ofstream file(_file);
    if (!file.is_open()) {
        std::cerr << "Failed to open file: " << _file << std::endl;
        return;
    }

    // Get the statistics
    std::vector<int> data = dataRep(userReports); 

    // Write the channel name
    file << "Channel " << _channel << "\n\n"; // Added extra newline for spacing
    
    // Write stats section
    file << "Stats:\n";
    file << "Total: " << userReports.size() << "\n";
    file << "active: " << data.at(0) << "\n";
    file << "forces arrival at scene: " << data.at(1) << "\n\n"; // Extra newline for spacing
    
    // Write event reports section
    file << "Event Reports:\n\n"; // Extra newline before the reports start
    for (size_t i = 0; i < userReports.size(); i++) {
        file << "Report_" << (i + 1) << ":\n";
        file << "\tcity: " << userReports.at(i).get_city() << "\n";  // Added tab before each line
        file << "\tdate time: " << epochToDate(userReports.at(i).get_date_time()) << "\n";  // Added tab before each line
        file << "\tevent name:" << userReports.at(i).get_name() << "\n";  // Added tab before each line
        file << "\tsummary: " << get_short_desc(userReports.at(i).get_description()) << "\n\n"; // Added tab before each line
    }

    // Close the file after writing
    file.close();
    std::cout << "Text file written successfully to: " << _file << std::endl;
}

        

std::string KeyBoardInput::epochToDate (std::time_t epochTime) 
{
    // Convert epoch time to a `tm` structure in local time
    std::tm* timeInfo = std::localtime(&epochTime);
    // Create a string stream to format the date-time
    std::ostringstream oss;
    // Format the date-time as "DD/MM/YY HH:MM"
    oss << std::put_time(timeInfo, "%d/%m/%y %H:%M");
    // Return the formatted string
    return oss.str();
}


void KeyBoardInput::splitter(const std::string& input)
{
    splitAction.clear();
    std::istringstream stream(input);  // Create a stream from the input string
    std::string word;
    while(stream>>word)
    {
        size_t colonPos = word.find(':');
        if (colonPos != std::string::npos)
        {
            // Split the word into two parts: before ':' and after ':'
            std::string part1 = word.substr(0, colonPos);  // Before ':'
            std::string part2 = word.substr(colonPos + 1);  // After ':'
            splitAction.push_back(part1);
            splitAction.push_back(part2);
        } 
        else
        {
            splitAction.push_back(word);  // Add the word to the vector as is
        }
    } 
}


