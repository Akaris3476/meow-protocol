#pragma once

#include <iostream>
#include <netinet/in.h>
#include <sys/socket.h>

#include <unistd.h>

class TcpSocket
{
private:
    int _clientSocket;

    bool isSocketValid()
    {
        if (_clientSocket >= 0) return true;

        std::cout << "Tried to call function with invalid socket" << std::endl;
        return false;
    }

public:
    TcpSocket() : _clientSocket(socket(AF_INET, SOCK_STREAM, 0)) {}

    explicit TcpSocket(int clientSocket) : _clientSocket(clientSocket) {}



    bool connectToServer(int port)
    {
        if (!isSocketValid()) return false;

        sockaddr_in serverAddress{};
        serverAddress.sin_family = AF_INET;
        serverAddress.sin_port = htons(port);
        serverAddress.sin_addr.s_addr = INADDR_ANY;

        if (connect(_clientSocket, reinterpret_cast<sockaddr*>(&serverAddress),
            sizeof(serverAddress)) < 0)
        {
            return false;
        }
        return true;
    }

    bool send(const char* msg)
    {
        if (!isSocketValid()) return false;

        return ::send(_clientSocket, msg, strlen(msg), 0) >= 0;
    }

    int send(const char* msg, size_t length) {
        if (!isSocketValid()) return false;

        return ::send(_clientSocket, msg, length, 0);
    }



    std::string receive()
    {
        if (!isSocketValid()) return "";

        char buffer[1024] = { };

        ssize_t len = recv(_clientSocket, buffer, sizeof(buffer) - 1, 0);

        if (len > 0) return std::string(buffer, len);

        return "";
    }

    ssize_t receive(char* buffer, size_t maxLen)
    {
        if (!isSocketValid()) return -1;

        ssize_t bytesRead = recv(_clientSocket, buffer, maxLen, 0);

        return bytesRead;
    }



    void close()
    {
        if (_clientSocket >= 0)
        {
            std::cout << "Closing client socket " << _clientSocket << std::endl;

            int clientSocketToClose = _clientSocket;
            _clientSocket = -1;
            ::close(clientSocketToClose);
        }

    }

    bool isRunning()
    {
        return _clientSocket >= 0;
    }

    ~TcpSocket()
    {
        close();
    }
};