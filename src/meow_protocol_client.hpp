#pragma once

#include "tcp/TcpServer.hpp"
#include "tcp/TcpSocket.hpp"


enum class MessageType : uint8_t {
    String = 1,
    Number = 2,
};

struct Message {
    MessageType type;
    std::vector<uint8_t> payload;

    std::string asString () const
    {
        if (type != MessageType::String) return std::string();


        return std::string(payload.begin(), payload.end());
    }

    int32_t asNumber() const
    {
        if (type != MessageType::Number) return 0;

        uint32_t num;
        std::memcpy(&num, payload.data(), sizeof(num));
        return static_cast<int32_t>(num);
    }


};


class MeowProtocolClient;

std::unique_ptr<MeowProtocolClient> acceptMeowClient(TcpServer& server)
{

    std::unique_ptr<TcpSocket> clientSocket = server.acceptConnection();

    if (!clientSocket) {
        return nullptr;
    }

    return std::make_unique<MeowProtocolClient>(std::move(clientSocket));
}


class MeowProtocolClient
{
private:
    std::unique_ptr<TcpSocket> _socket;

    bool readExact(uint8_t* dest, size_t length)
    {
        size_t totalBytesRead = 0;

        while (totalBytesRead < length)
        {
            ssize_t result = _socket->receive(
                reinterpret_cast<char*>(dest + totalBytesRead),
                length - totalBytesRead
            );

            if (result <= 0)
            {
                return false;
            }

            totalBytesRead += result;
        }

        return true;
    }
public:

    explicit MeowProtocolClient(std::unique_ptr<TcpSocket> socket) : _socket(std::move(socket)) {}

    // packet header - 4 bytes of payload size + 1 byte of type
    bool sendMsg(const Message& packet)
    {

        if (!_socket ) return false;

        auto payloadSize = static_cast<uint32_t>(packet.payload.size());
        auto netPayloadSize = htonl(payloadSize);

        auto rawType = static_cast<uint8_t>(packet.type);

        constexpr size_t headerSize = sizeof(netPayloadSize) + sizeof(rawType);

        size_t totalSize = payloadSize + headerSize;


        std::vector<uint8_t> buffer(totalSize);
        buffer[sizeof(netPayloadSize)] = rawType;

        std::memcpy(buffer.data(), &netPayloadSize, sizeof(netPayloadSize));

        if (payloadSize > 0)
        {
            std::memcpy(buffer.data() + headerSize, packet.payload.data(), payloadSize);
        }


        size_t bytesSent = 0;
        while (bytesSent < totalSize) {
            int result = _socket->send(
                reinterpret_cast<const char*>(buffer.data() + bytesSent),
                totalSize - bytesSent
            );

            if (result <= 0) {
                return false;
            }

            bytesSent += static_cast<size_t>(result);
        }

        return true;
    }


    std::optional<Message> receiveMsg()
    {
        if (!_socket) return std::nullopt;

        constexpr size_t headerSize = sizeof(uint32_t) + sizeof(MessageType);
        uint8_t headerBuffer[headerSize];

        if (!readExact(headerBuffer, headerSize))
        {
            return std::nullopt;
        }

        uint32_t netPayloadSize = 0;
        std::memcpy(&netPayloadSize, headerBuffer, sizeof(netPayloadSize));

        uint32_t payloadSize = ntohl(netPayloadSize);
        auto type = static_cast<MessageType>(headerBuffer[4]);



        Message msg;
        msg.type = type;

        if (payloadSize > 0)
        {
            msg.payload.resize(payloadSize);

            if (!readExact(msg.payload.data(), payloadSize))
            {
                return std::nullopt;
            }
        }

        return msg;
    }

    void close()
    {
        if (!_socket) return;

        _socket->close();
        _socket.reset();


    }



    bool isRunning()
    {
        return _socket->isRunning();
    }

};
