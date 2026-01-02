package bgu.spl.net.srv;

//import java.io.IOException;
import java.util.concurrent.ConcurrentHashMap;

public class ConnectionsImpl<T> implements Connections<T> {
    // Logic and Data Structures of the server
    int connectionIdGen = 0;
    long msgIdGen = 0;
    ConcurrentHashMap <Integer, ConnectionHandler<T>> connectionHandlers;
    ConcurrentHashMap <String, String> usernamePasscode;
    ConcurrentHashMap <String, String> usernamePasscodeOnline;
    ConcurrentHashMap <Integer, String> connectionIdToUsername;
    ConcurrentHashMap <String, ConcurrentHashMap<Integer, String>> channels; // channel TO connectionId, (subscriberId MAYBE NOT BE NEEDED)

    public ConnectionsImpl() {
        connectionHandlers = new ConcurrentHashMap<>();
        usernamePasscode = new ConcurrentHashMap<>();
        usernamePasscodeOnline = new ConcurrentHashMap<>();
        connectionIdToUsername = new ConcurrentHashMap<>();
        channels = new ConcurrentHashMap<>();
    }
    
    public synchronized boolean send(int connectionId, T msg) { 
        ConnectionHandler<T> connectionHandler = connectionHandlers.get(connectionId); // making sure it wasnt removed mid iterating
        if (connectionHandler != null){
            connectionHandler.send(msg);
            return true;
        }
        return false;
    }

    public synchronized void send(String channel, T msg) { // NOT USED
        ConcurrentHashMap<Integer, String> channelSubscribers = channels.get(channel);
        for (Integer connectionId : channelSubscribers.keySet()) {
            // ADD each Subscription (Id - it is sent to) header to the msg
            send(connectionId, msg);
        }
    }

    public synchronized void disconnect(int connectionId) { // ShouldTerminate will probably be changed by the protocol when sees a disconnect frame
        // NEED TO SEND FRAME TO CLIENT???
        usernamePasscodeOnline.remove(connectionIdToUsername.get(connectionId)); // remove from online users
        connectionIdToUsername.remove(connectionId); // remove from list of connections to usernames
        for (ConcurrentHashMap<Integer, String> channel : channels.values()) { // remove subscriptions
            channel.remove(connectionId);
        }
        connectionHandlers.remove(connectionId); // remove from connectionHandlers List
    } 

    public synchronized void addConnectionHandler(ConnectionHandler<T> connectionHandler) {
        int id = generateConnectionId(); // generating server unique id for the connectionHandler
        connectionHandler.startProtocol(id, this); // starting the protocol
        connectionHandlers.putIfAbsent(id, connectionHandler); // adds the connectionHandler to the map if not already there    
    }

    public synchronized void subscribe(String channelName, int connectionId, String subscriberId) {
        if (channels.containsKey(channelName)) {
            channels.get(channelName).put(connectionId, subscriberId); // add a subscriber to the targeted channel
        }
        else { // create a new channel
            channels.put(channelName, new ConcurrentHashMap<>());
            channels.get(channelName).put(connectionId, subscriberId); 
        }
    }

    public synchronized void unSubscribe(String channelName, int connectionId) {
        ConcurrentHashMap<Integer, String> channel = channels.get(channelName);
        if (channel != null && channel.containsKey(connectionId)) {
            channel.remove(connectionId);
        }
    }
    
    public synchronized boolean isLogged(String username) {
        return usernamePasscodeOnline.containsKey(username);
    }

    public synchronized boolean login(int connectionId, String username, String passcode) { // order should be changed of puting in lists
        String savedPasscode = usernamePasscode.putIfAbsent(username, passcode);
        if (savedPasscode != null && !savedPasscode.equals(passcode)) {
            // ERROR - WRONG PASSCODE
            return false;
        }
        usernamePasscodeOnline.put(username, passcode); // make him online
        connectionIdToUsername.put(connectionId, username); // for later to disconnect
        return true;
    }

    public synchronized ConcurrentHashMap<Integer, String> getChannelSubscriptionsId(String channelName) {
        return channels.get(channelName);
    }
    
    public synchronized long generateMsgId(){
        msgIdGen++;
        return msgIdGen;
    }

    private synchronized int generateConnectionId(){
        connectionIdGen++;
        return connectionIdGen;
    }
}
