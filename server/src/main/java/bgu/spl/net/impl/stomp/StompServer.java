package bgu.spl.net.impl.stomp;

import bgu.spl.net.srv.Server;

public class StompServer {

    public static void main(String[] args) { 
        // args[0] = port, args[1] = tpc / reactor
        // Based on EchoServer
        if (args[1].equals("tpc")){
            Server.threadPerClient(
                Integer.parseInt(args[0]), //port
                () -> new StompMessagingProtocolImpl(), //protocol factory
                FrameEncoderDecoder::new //message encoder decoder factory
            ).serve();
        }
        else if (args[1].equals("reactor")){
            Server.reactor(
                 Runtime.getRuntime().availableProcessors(),
                 Integer.parseInt(args[0]), //port
                 () -> new StompMessagingProtocolImpl(), //protocol factory
                 FrameEncoderDecoder::new //message encoder decoder factory
            ).serve();
        }
        else{
            System.out.println("args[1] should be tpc / reactor");
        }
    }
}
