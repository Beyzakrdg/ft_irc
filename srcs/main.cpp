#include "../includes/Server.hpp"

int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        std::cerr << "Please use this command: ./ircserv <port> <password>" << std::endl;
        return (1);
    }
    std::string portStr = argv[1];
    for (size_t i = 0; i < portStr.length(); i++)
    {
        if (!std::isdigit(portStr[i]))
        {
            std::cerr << "Error: Port must be a number" << std::endl;
            return (1);
        }
    }
    int port = std::atoi(argv[1]);
    if (port <= 0 || port > 65535)
    {
        std::cerr << "Error: Invalid port number (must be 1-65535)" << std::endl;
        return (1);
    }
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
