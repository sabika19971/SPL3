package bgu.spl.net.srv;

//import java.io.IOException;
import java.util.concurrent.ConcurrentHashMap;

public interface Connections<T> {

    boolean send(int connectionId, T msg);

    void send(String channel, T msg);

    void disconnect(int connectionId);

    // WE ADDED
    void addConnectionHandler(ConnectionHandler<T> connectionHandler);

    void subscribe(String channelName, int connectionId, String subscriberId);

    void unSubscribe(String channelName, int connectionId);

    boolean isLogged(String username);

    boolean login(int connectionId, String username, String passcode);
    
    ConcurrentHashMap<Integer, String> getChannelSubscriptionsId(String channelName);

    long generateMsgId();
}
