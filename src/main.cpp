#include <iostream>
#include <thread>

#include <netinet/in.h>

#include "tcp/TcpServer.hpp"
#include "meow_protocol_client.hpp"

void createClientSocketAndSendMessage(const int PORT, u_int8_t num_of_messages)
{
    std::cout << "Client: connecting to server..." << std::endl;

    TcpSocket client;
    if (client.connectToServer(PORT)) {
        std::cout << "Client: Connection established" << std::endl;

        for (int i = 0; i < num_of_messages; i++)
        {
            std::cout << std::endl << "Message " << i + 1 << std::endl;

            client.send("Hello server!");
            std::cout << "Client: message sent" << std::endl;

            std::string response = client.receive();
            std::cout << "Client: response from server: \"" << response << "\"" << std::endl;
        }

    } else {
        std::cout << "Client: error connecting to server" << std::endl;
    }

    client.close();
}

void createMeowClientSocketAndSendMessage(const int PORT, MessageType type, u_int8_t num_of_messages)
{
    auto rawSocket = std::make_unique<TcpSocket>();

    if (!rawSocket->connectToServer(PORT))
    {
        std::cout << std::format("MeowClient: Could not connect to server!") << std::endl;
        rawSocket->close();
        return;
    }

    MeowProtocolClient meowClient(std::move(rawSocket));

    for (int i = 0; i < num_of_messages; i++)
    {
        std::cout << "Message " << i+1 << std::endl;

        Message msg;
        if (type == MessageType::Number)
        {
            int numToSend = 100 * (i+1);
            msg = Message::makeNum(numToSend);

            std::cout << "MeowClient: Sending number " << numToSend << "..." << std::endl;
        }
        else if (type == MessageType::String)
        {

            msg = Message::makeStr("Meow Protocol Test " + std::to_string(i+1));

            std::cout << "MeowClient: Sending string..." << std::endl;
        }

        meowClient.sendMsg(msg);


        if (std::optional<Message> res = meowClient.receiveMsg())
        {
            switch (res->type)
            {
                case MessageType::String:
                    std::cout << std::format("MeowClient: Response from server: {}", res->asString()) << std::endl;
                    break;
                case MessageType::Number:
                    std::cout << std::format("MeowClient: Response from server: {}",  res->asNumber()) << std::endl;
                    break;
            }
        }
        else
        {
            std::cout << "MeowClient: Failed to receive response from server" << std::endl;
        }


    }

    meowClient.close();

}

void startServerAndListen(TcpServer& server)
{
    if (!server.start()) {
        std::cout << "Server did not start" << std::endl;
        return;
    }
    std::cout << "Server started and listens" << std::endl;

    while (server.isRunning())
    {
        std::unique_ptr<TcpSocket> clientConn = server.acceptConnection();

        if (!clientConn || !clientConn->isRunning()) break;

        std::cout << "Server: Client connected successfully" << std::endl;

        int num_of_messages = 0;
        while (clientConn->isRunning())
        {
            std::string msg = clientConn->receive();

            if (msg.empty()) break;

            num_of_messages += 1;

            std::cout << "Server: got from client: \"" << msg << "\"" << std::endl;

            std::string response = std::format("Hello from server! Response count: {}", num_of_messages);
            clientConn->send(response.c_str());
            std::cout << "Server: response's been sent" << std::endl;

        }


    }
}

void startMeowServerAndListen(TcpServer& server)
{
        server.start();

        while (server.isRunning())
        {
            std::unique_ptr<TcpSocket> clientConn = server.acceptConnection();

            if (!clientConn || !clientConn->isRunning()) break;

            std::cout << "MeowServer: Client connected successfully" << std::endl;

            MeowProtocolClient meowClient(std::move(clientConn));


            int num_of_messages = 0;
            while (meowClient.isRunning())
            {
                auto msgOpt = meowClient.receiveMsg();

                if (!msgOpt.has_value()) break;

                const Message& msg = msgOpt.value();

                num_of_messages += 1;

                switch (msg.type)
                {
                case MessageType::String:
                    {
                        std::string text(msg.payload.begin(), msg.payload.end());
                        std::cout << "Server: got STRING from client: \"" << text << "\"" << std::endl;

                        std::string response = std::format("Hello from server! Response count: {}", num_of_messages);

                        Message resMsg = Message::makeStr(response);

                        if (!meowClient.sendMsg(resMsg)) {
                            std::cout << "Server: failed to send response" << std::endl;
                            break;
                        }

                        std::cout << "Server: response's been sent" << std::endl;
                        break;
                    }

                case MessageType::Number:
                {
                    if (msg.payload.size() >= sizeof(int32_t)) {
                        int32_t num;
                        std::memcpy(&num, msg.payload.data(), sizeof(num));

                        std::cout << "Server: got NUMBER from client: " << num << std::endl;

                        Message resMsg = Message::makeNum(num*2);

                        meowClient.sendMsg(resMsg);
                        std::cout << "Server: doubled number response's been sent" << std::endl;
                    }
                    break;
                }
                }

            }


        }

}


int main()
{
    const int PORT = 9997;

    TcpServer server(PORT);

    std::thread serverThread([&server] {

        startServerAndListen(server);

        std::cout << "Finishing server thread" << std::endl;
    });


    std::cout << std::endl << "Testing Tcp Wrappers" << std::endl << std::endl;

    std::cout << std::endl << "Client 1: " << std::endl << std::endl;
    createClientSocketAndSendMessage(PORT, 1);


    std::cout << std::endl << "Client 2: " << std::endl << std::endl;
    createClientSocketAndSendMessage(PORT, 2);

    std::this_thread::sleep_for(std::chrono::milliseconds(500));


    server.close();

    if (serverThread.joinable())
    {
        serverThread.join();
    }




    TcpServer MeowServer(PORT);

    std::cout << std::endl << std::endl << "Testing Meow Protocol" << std::endl << std::endl;

    std::thread meowServerThread([&MeowServer] {
        startMeowServerAndListen(MeowServer);

        std::cout << "Finishing meow server thread" << std::endl;
    });


    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    std::cout << std::endl << "Client 1: " << std::endl << std::endl;
    createMeowClientSocketAndSendMessage(PORT, MessageType::String, 3);


    std::cout << std::endl << std::endl << "Client 2: " << std::endl << std::endl;
    createMeowClientSocketAndSendMessage(PORT, MessageType::Number, 2);



    MeowServer.close();

    if (meowServerThread.joinable())
    {
        meowServerThread.join();
    }



    std::cout << "\nFinish" << std::endl;
    return 0;
}