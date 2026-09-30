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


int main()
{
    const int PORT = 9997;

    TcpServer server(PORT);

    std::thread serverThread([&server] {

        startServerAndListen(server);

        std::cout << "Finishing server thread" << std::endl;
    });

    // createClientSocketAndSendMessage(PORT, 1);
    //
    //
    // std::this_thread::sleep_for(std::chrono::milliseconds(500));
    //
    // std::cout << std::endl << "Client 2: " << std::endl << std::endl;
    //
    //
    // createClientSocketAndSendMessage(PORT, 2);
    //
    // std::this_thread::sleep_for(std::chrono::milliseconds(500));
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    server.close();

    if (serverThread.joinable())
    {
        serverThread.join();
    }

    TcpServer MeowServer(PORT);


    std::cout << std::endl << "Testing Meow Protocol" << std::endl << std::endl;

    std::thread meowServerThread([&MeowServer] {

        TcpServer& server = MeowServer ;
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

                        Message responseMsg{ MessageType::String };
                        responseMsg.payload.assign(response.begin(), response.end());

                        if (!meowClient.sendMsg(responseMsg)) {
                            std::cout << "Server: failed to send response" << std::endl;
                            break;
                        }

                        std::cout << "Server: response's been sent" << std::endl;
                        break;
                    }

                case MessageType::Number:
                {
                    if (msg.payload.size() >= sizeof(int32_t)) {
                        uint32_t netNum;
                        std::memcpy(&netNum, msg.payload.data(), sizeof(netNum));
                        int32_t num = static_cast<int32_t>(netNum);

                        std::cout << "Server: got NUMBER from client: " << num << std::endl;

                        int32_t respNum = num * 2;
                        uint32_t netRespNum = static_cast<uint32_t>(respNum);

                        Message responseMsg{ MessageType::Number };
                        responseMsg.payload.resize(sizeof(netRespNum));
                        std::memcpy(responseMsg.payload.data(), &netRespNum, sizeof(netRespNum));

                        meowClient.sendMsg(responseMsg);
                        std::cout << "Server: doubled number response's been sent" << std::endl;
                    }
                    break;
                }
                }

            }


        }

        std::cout << "Finishing meow server thread" << std::endl;
    });




    std::this_thread::sleep_for(std::chrono::milliseconds(500));


    {
        auto rawSocket = std::make_unique<TcpSocket>();
        if (rawSocket->connectToServer(PORT))
        {
            MeowProtocolClient meowClient(std::move(rawSocket));

            std::cout << std::endl << "Client 1: " << std::endl << std::endl;
            std::cout << "Message 1" << std::endl;

            Message numMsg;
            numMsg.type = MessageType::Number;
            uint32_t netNum = 100;
            numMsg.payload.resize(sizeof(netNum));
            std::memcpy(numMsg.payload.data(), &netNum, sizeof(netNum));

            std::cout << "MeowClient: Sending number 100..." << std::endl;
            meowClient.sendMsg(numMsg);

            std::optional<Message> res1 =  meowClient.receiveMsg();

            if (res1.has_value() && res1.value().type == MessageType::Number)
            {
                int parsedRes1 = res1.value().asNumber();
                std::cout << std::format("MeowClient: Response from server: {}", parsedRes1) << std::endl;

            }
            else if (res1.has_value())
            {
                std::string parsedRes1 = res1.value().asString();
                std::cout << std::format("MeowClient: Response from server: {}", parsedRes1) << std::endl;
            }

            meowClient.close();

        } else
        {
            std::cout << std::format("MeowClient: Could not connect to server!") << std::endl;
        }


        std::cout << std::endl << std::endl << "Client 2: " << std::endl << std::endl;

        auto rawSocket2 = std::make_unique<TcpSocket>();
        if (rawSocket2->connectToServer(PORT))
        {

            MeowProtocolClient meowClient2(std::move(rawSocket2));


            std::cout  << "Message 2" << std::endl;

            Message strMsg;
            strMsg.type = MessageType::String;
            std::string msg = "Meow Protocol Test";
            strMsg.payload.assign(msg.begin(), msg.end());

            std::cout << "MeowClient: Sending string..." << std::endl;
            meowClient2.sendMsg(strMsg);

            std::optional<Message> res2 =  meowClient2.receiveMsg();

            if (res2.has_value() && res2.value().type == MessageType::Number)
            {
                int parsedRes2 = res2.value().asNumber();
                std::cout << std::format("MeowClient2: Response from server: {}", parsedRes2) << std::endl;

            }
            else if (res2.has_value())
            {
                std::string parsedRes2 = res2.value().asString();
                std::cout << std::format("MeowClient2: Response from server: {}", parsedRes2) << std::endl;
            }

            std::cout  << "Message 3" << std::endl;

            Message strMsg2;
            strMsg2.type = MessageType::String;
            std::string msg2 = "Meow Protocol Test 2";
            strMsg2.payload.assign(msg2.begin(), msg2.end());

            std::cout << "MeowClient2: Sending string..." << std::endl;
            meowClient2.sendMsg(strMsg2);

            std::optional<Message> res3 =  meowClient2.receiveMsg();

            if (res3.has_value() && res3.value().type == MessageType::Number)
            {
                int parsedRes3 = res3.value().asNumber();
                std::cout << std::format("MeowClient2: Response from server: {}", parsedRes3) << std::endl;

            }
            else if (res3.has_value())
            {
                std::string parsedRes3 = res3.value().asString();
                std::cout << std::format("MeowClient2: Response from server: {}", parsedRes3) << std::endl;
            }
        }
        else
        {
            std::cout << std::format("MeowClient2: Could not connect to server!");
        }


    }

    MeowServer.close();

    if (meowServerThread.joinable())
    {
        meowServerThread.join();
    }





    std::cout << "\nFinish" << std::endl;
    return 0;
}