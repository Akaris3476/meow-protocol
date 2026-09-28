#pragma once

#include <iostream>
#include <netinet/in.h>
#include <sys/socket.h>

#include <unistd.h>
#include "TcpSocket.hpp"

class TcpServer
{
private:
    int _port;
    int _serverSocket;
    sockaddr_in _serverAddress {};
public:

    explicit TcpServer(int port) : _port(port), _serverSocket(-1)
    {
        _serverAddress.sin_family = AF_INET;
        _serverAddress.sin_port = htons(port);
        _serverAddress.sin_addr.s_addr = INADDR_ANY;
    }

    bool start()
    {
        _serverSocket = socket(AF_INET, SOCK_STREAM, 0);

        if (bind(_serverSocket, reinterpret_cast<sockaddr*>(&_serverAddress),
             sizeof(_serverAddress)) < 0)
        {
            std::cout << "Bind failed" << std::endl;
            ::close(_serverSocket);
            return false;
        }

        if (listen(_serverSocket, 5) < 0)
        {
            std::cout << "Listen failed" << std::endl;
            ::close(_serverSocket);
            return false;
        }

        return true;
    }

    std::unique_ptr<TcpSocket> acceptConnection()
    {
        if (!isRunning())
        {
            std::cout << "Accept connection failed. Server Socket is closed" << std::endl;
            return nullptr;
        }

        int clientSocket = accept(_serverSocket, nullptr, nullptr);

        if (clientSocket < 0)
        {
            if (!isRunning())
            {
                std::cout << "Attempt to accept connection stopped: "
                             "Server socket has been closed" << std::endl;
                return nullptr;
            }

            std::cout << "Accept connection failed. Client socket is invalid" << std::endl;
            return nullptr;
        }


        return std::make_unique<TcpSocket>(clientSocket);
    }





    bool isRunning()
    {
        return _serverSocket >= 0;
    }

    ~TcpServer()
    {
        close();
    }
    void close()
    {
        if (_serverSocket >= 0)
        {
            std::cout << "Closing server..." << std::endl;

            int serverSocketToClose = _serverSocket;
            _serverSocket = -1;
            ::close(serverSocketToClose);

        }
    }
};



