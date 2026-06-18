#include "../includes/Server.hpp"
#include <iostream>
#include <cstdlib>

int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        std::cerr << "Please use this command: ./ircserv <port> <password>" << std::endl;
        return (1);
    }
    int port = std::atoi(argv[1]);
    std::string password = argv[2];
    try
    {
        Server ircServer(port, password);
        ircServer.init();
        ircServer.run();
    }
    catch (const std::exception &exception)
    {
        std::cerr << "Error: " << exception.what() << std::endl;
        return (1);
    }
    return (0);
}
