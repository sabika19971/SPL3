package bgu.spl.net.impl.stomp;

import java.util.HashMap;
import java.util.concurrent.ConcurrentHashMap;

import bgu.spl.net.api.StompMessagingProtocol;
import bgu.spl.net.srv.Connections;

public class StompMessagingProtocolImpl implements StompMessagingProtocol<String>{

    private boolean shouldTerminate = false;
    private int connectionId;
    private Connections<String> connections;
    private HashMap<String, String> subscriptions = new HashMap<>(); // subscriber id TO channel name
    private boolean isLoggedIn = false;

    public void start(int connectionId, Connections<String> connections) {
        this.connectionId = connectionId;
        this.connections = connections;
    }
    
    public void process(String message) {
        String[] lines = message.split("\n");
        if (lines[0].equals("CONNECT")) { 
            processCONNECTframe(message);
        }
        else if (!isLoggedIn) {
            // ERROR - CAN ONLY CONNECT IF ISNT LOGGED IN
        }
        else if (lines[0].equals("SUBSCRIBE")) { 
            processSUBSCRIBEframe(message);
        }
        else if (lines[0].equals("UNSUBSCRIBE")) { 
            processUNSUBSCRIBEframe(message);      
        }
        else if (lines[0].equals("SEND")) { 
            processSENDframe(message);
        }
        else if (lines[0].equals("DISCONNECT")) { 
            processDISCONNECTframe(message);
        }
        else {
            // ERROR - ILLEGAL FRAME NAME
        }
    }

	private void produceAndSendErrorFrame(String message, String receiptId, String errorCausingFrame, String description, boolean disconnect) {
        errorCausingFrame = errorCausingFrame.substring(0, errorCausingFrame.lastIndexOf("\n")); // removing the \u0000
        connections.send(connectionId, "ERROR\n"
                                        +"receipt-id:"+receiptId+"\n" 
                                        +"message:"+message+"\n\n"
                                        +"The message:\n"
                                        +"-----\n"
                                        +errorCausingFrame+"\n"
                                        +"-----\n"
                                        +description
                                        +"\n");
        if (disconnect) {
            connections.disconnect(connectionId);
            shouldTerminate = true;
        }
    }

    private String produceReceiptFrame(String receiptId, String messageToPrint) {
        return "RECEIPT\n"
               +"receipt-id:"+receiptId+"\n"
               +messageToPrint
               +"\n\n";
    }

    private void processCONNECTframe(String msg) { // 4 headers 0 body + receipt header
        String[] lines = msg.split("\n");
        String version = "";
        String host = "";
        String login = "";
        String passcode = "";
        String receipt = "";
        boolean incorrectHeaders = false;
        for (int i = 1; i < lines.length && !lines[i].equals(""); i++) {
            if (lines[i].split(":")[0].equals("accept-version")) {
                version = lines[i].split(":")[1];
            }
            else if (lines[i].split(":")[0].equals("host")) {
                host = lines[i].split(":")[1];
            }
            else if (lines[i].split(":")[0].equals("login")) {
                login = lines[i].split(":")[1];
            }
            else if (lines[i].split(":")[0].equals("passcode")) {
                passcode = lines[i].split(":")[1];
            }
            else if (lines[i].split(":")[0].equals("receipt")) {
                receipt = lines[i].split(":")[1];
            }
            else{ // header that doesnt belong to that frame or spelled incorrect
                incorrectHeaders = true;
            }
        }
        // NOW HEADERS ARE parsed
        if (incorrectHeaders) {
            produceAndSendErrorFrame("Illegal headers structure", receipt, msg,
                            "one or more of the headers names are written incorrect",
                             false);
            shouldTerminate = true;
        }
        else if (version.isEmpty() || host.isEmpty() || login.isEmpty() || passcode.isEmpty()) { // missing headers
            produceAndSendErrorFrame("Illegal headers structure", receipt, msg,
                            "one or more of the headers names are missing",
                             false);
            shouldTerminate = true;
        }
        else if ( !version.equals("1.2")) {
            produceAndSendErrorFrame("Illegal version header value", receipt, msg,
                        "version must be 1.2", false);
            shouldTerminate = true;
        }
        else if ( !host.equals("stomp.cs.bgu.ac.il")) { 
            produceAndSendErrorFrame("Illegal host value", receipt, msg,
                        "host must be stomp.cs.bgu.ac.il", false);
            shouldTerminate = true;
        }
        else if (connections.isLogged(login)) {
            produceAndSendErrorFrame("User already logged in", receipt, msg,
                        "User already logged in", false);
            shouldTerminate = true;
        }
        else if ( !connections.login(connectionId, login, passcode)) {
            produceAndSendErrorFrame("Wrong password", receipt, msg,"Wrong password", false);
            shouldTerminate = true;
        }
        else {
            isLoggedIn = true; // indicates that the user is connected - CLIENT SIDE AND REMOVE FROM HERE
            String connectedFrame = "CONNECTED\nversion:1.2\n\n"; // \u0000 will be added in the encdec
            connections.send(connectionId, connectedFrame);
            if ( !receipt.equals("")) {
                connections.send(connectionId, produceReceiptFrame(receipt, ""));
            }
        }
    }

    private void processSUBSCRIBEframe(String msg) { // 2 headers 0 body + receipt header
        String[] lines = msg.split("\n");
        String destination = "";
        String id = "";
        String receipt = "";
        boolean incorrectHeaders = false;
        for (int i = 1; i < lines.length && !lines[i].equals(""); i++) {
            if (lines[i].split(":")[0].equals("destination")) {
                destination = lines[i].split(":")[1]; // WAS "/"+lines[i].split(":")[1];
            }
            else if (lines[i].split(":")[0].equals("id")) {
                id = lines[i].split(":")[1];
            }
            else if (lines[i].split(":")[0].equals("receipt")) {
                receipt = lines[i].split(":")[1];
            }
            else{ // header that doesnt belong to that frame or spelled incorrect
                incorrectHeaders = true;
            }
        }
        // NOW HEADERS ARE parsed
        if (incorrectHeaders) {
            produceAndSendErrorFrame("Illegal headers structure", receipt, msg,
                            "one or more of the headers names are written incorrect",
                             true);
        }
        else if (destination.isEmpty() || id.isEmpty()) { // missing headers
            produceAndSendErrorFrame("Illegal headers structure", receipt, msg,
                            "one or more of the headers names are missing",
                             true);
        }
        else if (subscriptions.putIfAbsent(id, destination) != null) { // for unsubscribing later 
            produceAndSendErrorFrame("Illegal subscription Id", receipt, msg,
                            "the id provided already assigned to a different channel", 
                            true);
        }
        else {
            connections.subscribe(destination, connectionId, id); // id = subscriptionId
            if ( !receipt.equals("")) {
                connections.send(connectionId, produceReceiptFrame(receipt, "Joined channel " + destination.substring(1)));
            }   
        }
    }

    private void processUNSUBSCRIBEframe(String msg) { // 1 headers 0 body + receipt header
        String[] lines = msg.split("\n");
        String id = "";
        String receipt = "";
        boolean incorrectHeaders = false;
        for (int i = 1; i < lines.length && !lines[i].equals(""); i++) {
            if (lines[i].split(":")[0].equals("id")) {
                id = lines[i].split(":")[1];
            }
            else if (lines[i].split(":")[0].equals("receipt")) {
                receipt = lines[i].split(":")[1];
            }
            else{ // header that doesnt belong to that frame or spelled incorrect
                incorrectHeaders = true;
            }
        }
        // NOW HEADERS ARE parsed
        if (incorrectHeaders) {
            produceAndSendErrorFrame("Illegal headers structure", receipt, msg,
                            "one or more of the headers names are written incorrect",
                             true);
        }
        else if (id.isEmpty()) { // missing headers
            produceAndSendErrorFrame("Illegal headers structure", receipt, msg,
                            "one or more of the headers names are missing",
                             true);
        }
        else {
            String channelName = subscriptions.get(id);
            connections.unSubscribe(channelName, connectionId); // doesnt throw exception ##############
            if ( !receipt.equals("")) {
                connections.send(connectionId, produceReceiptFrame(receipt, "Exited channel " + channelName.substring(1)));
            }
        }      
    }

    private void processSENDframe(String msg) { // 1 header n body + receipt header
        String[] lines = msg.split("\n");
        String destination = "";
        String receipt = "";
        int descriptionStartIndex = 0;
        boolean incorrectHeaders = false;
        for (int i = 1; i < lines.length && !lines[i].equals(""); i++) {
            if (lines[i].split(":")[0].equals("destination")) {
                destination = lines[i].split(":")[1];
            }
            else if (lines[i].split(":")[0].equals("receipt")) {
                receipt = lines[i].split(":")[1];
            }
            else{
                incorrectHeaders = true;
            }
            descriptionStartIndex = i + 1;
        }  
        // NOW HEADERS ARE parsed
        if (incorrectHeaders) {
            produceAndSendErrorFrame("Illegal headers structure", receipt, msg,
                            "one or more of the headers names are written incorrect",
                             true);
        }
        else if (destination.isEmpty()) { // missing headers
            produceAndSendErrorFrame("Illegal headers structure", receipt, msg,
                            "one or more of the headers names are missing",
                             true);
        }
        else if ( !subscriptions.values().contains(destination)) { 
            produceAndSendErrorFrame("User isnt subscribed to channel: "+destination, receipt, msg,
                            "user must be subscribed to a channel in order to send messages to it",
                             true);
        }
        else {
            if ( !receipt.equals("")) {
                connections.send(connectionId, produceReceiptFrame(receipt, ""));
            }
            // MODIFY THE MESSAGE AS NEEDED
            String modifiedMessage = "MESSAGE\nmessage-id:" + connections.generateMsgId() + "\ndestination:" + destination + "\n"; // @@@@@@
            for (int i = descriptionStartIndex; i < lines.length; i++) { // attaching the body without the \u0000 at the end
                modifiedMessage += lines[i] + "\n";
            } 
            // to add subscriptionId per msg - start
            ConcurrentHashMap<Integer, String> connectionToSubscription = connections.getChannelSubscriptionsId(destination);
            for (ConcurrentHashMap.Entry<Integer, String> entry : connectionToSubscription.entrySet()) {
                String modifiedMsgPerSubscription = "MESSAGE\nsubscription:" + entry.getValue() + "\n"
                + modifiedMessage.substring(8);
                connections.send(entry.getKey(), modifiedMsgPerSubscription); // checks if still subscribed - no synchronze needed
            }
            // to add subscriptionId per msg - end
        } 
    }

    private void processDISCONNECTframe(String msg) { // 1 headers 0 body
        String[] lines = msg.split("\n");
        String receipt = "";
        if (lines[1].split(":")[0].equals("receipt")) {
            receipt = lines[1].split(":")[1];

            String receiptFrame = produceReceiptFrame(receipt, "logged out succesfully"); //"RECEIPT\nreceipt-id:" + receipt + "\n\n"; // \u0000 will be added in the encdec
            connections.send(connectionId, receiptFrame);
            connections.disconnect(connectionId); // why is it needed?
            shouldTerminate = true;
        }
        else{
            produceAndSendErrorFrame("Illegal headers structure", receipt, msg,
                            "one or more of the headers names are missing or written incorrect",
                             true);
        }     
    }

    public boolean shouldTerminate() {
        return shouldTerminate;
    }
    
}
