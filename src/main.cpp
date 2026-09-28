#include <iostream>
#include <thread>

#include <netinet/in.h>

#include "tcp/TcpServer.hpp"


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

int main()
{
    const int PORT = 9999;

    TcpServer server(PORT);

    std::thread serverThread([&server] {

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

                std::string response = std::format("Hello from server {}!", num_of_messages);
                clientConn->send(response.c_str());
                std::cout << "Server: response's been sent" << std::endl;

            }


        }

        std::cout << "Finishing server thread" << std::endl;
    });

    createClientSocketAndSendMessage(PORT, 1);


    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    std::cout << std::endl << "Client 2: " << std::endl << std::endl;


    createClientSocketAndSendMessage(PORT, 2);

    std::this_thread::sleep_for(std::chrono::milliseconds(500));


    server.close();


    if (serverThread.joinable()) {
        serverThread.join();
    }

    std::cout << "\nFinish" << std::endl;
    return 0;
}