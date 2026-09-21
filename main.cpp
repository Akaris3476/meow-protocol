#include <iostream>
#include <thread>

#include <netinet/in.h>
#include <sys/socket.h>

#include "client.cpp"
#include "server.cpp"

int main()
{
    int port = 9999;

    std::thread ta = std::thread(openServer, port);
    std::thread tb = std::thread(createClient, port);

    ta.join();
    tb.join();

    return 0;
}