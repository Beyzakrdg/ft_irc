#include "../includes/Server.hpp"

Server::Server(int port, std::string password) : portNo(port), serverFd(-1), psswd(password)
{
}

Server::~Server()
{
    if (serverFd != -1)
        close(serverFd);
}

void Server::init()
{
    serverFd = socket(AF_INET, SOCK_STREAM, 0);
    if (serverFd < 0)
        throw std::runtime_error("Socket creation failed");

    int opt = 1;
    setsockopt(serverFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    fcntl(serverFd, F_SETFL, O_NONBLOCK);

    struct sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(portNo);

    if (bind(serverFd, (struct sockaddr *)&address, sizeof(address)) < 0)
        throw std::runtime_error("Bind failed");

    if (listen(serverFd, 10) < 0)
        throw std::runtime_error("Listen failed");

    struct pollfd serverPollFd;
    serverPollFd.fd = serverFd;
    serverPollFd.events = POLLIN;
    fds.push_back(serverPollFd);
    
    std::cout << "Server started on port " << portNo << std::endl;
}

void Server::run()
{
    while (true)
    {
        if (poll(&fds[0], fds.size(), -1) < 0)
            throw std::runtime_error("Poll failed");

        for (size_t i = 0; i < fds.size(); i++)
        {
            if (fds[i].revents & POLLIN)
            {
                if (fds[i].fd == serverFd)
                {
                    acceptConnection();
                }
                else
                {
                    if (getClientData(fds[i].fd) == false)
                    {
                        fds.erase(fds.begin() + i);
                        i--;
                    }
                }
            }
        }
    }
}

bool Server::getClientData(int sockFd)
{
    char buff[1024];
    int countByte = recv(sockFd, buff, sizeof(buff) - 1, 0);
    size_t pos;

    if (countByte <= 0)
    {
        std::cout << "Client baglantisi koptu: " << sockFd << std::endl;
        close(sockFd);
        clientBuff.erase(sockFd);
        clients.erase(sockFd);
        return false; 
    }
    buff[countByte] = '\0';
    clientBuff[sockFd] += buff;
    while ((pos = clientBuff[sockFd].find('\n')) != std::string::npos)
    {
        std::string line = clientBuff[sockFd].substr(0, pos);
        if (!line.empty() && line[line.length() - 1] == '\r')
            line.erase(line.length() - 1);
        parseMessage(sockFd, line);
        clientBuff[sockFd].erase(0, pos + 1);
    }
    return true;
}

void Server::acceptConnection()
{
    struct sockaddr_in clientAddr;
    socklen_t clientLen = sizeof(clientAddr);
    int sockFd = accept(serverFd, (struct sockaddr *)&clientAddr, &clientLen);

    if (sockFd < 0)
    {
        std::cerr << "Accept failed" << std::endl;
        return;
    }

    fcntl(sockFd, F_SETFL, O_NONBLOCK);

    struct pollfd clientPollFd;
    clientPollFd.fd = sockFd;
    clientPollFd.events = POLLIN;
    fds.push_back(clientPollFd);
    
    clients.insert(std::make_pair(sockFd, Client(sockFd)));

    std::cout << "New client connected: " << sockFd << std::endl;
}

void Server::parseMessage(int sockFd, std::string line)
{
    if (line.empty())
        return;

    std::string cmd;
    std::vector<std::string> args;
    size_t i = 0;

    while (i < line.size() && (line[i] == ' ' || line[i] == '\t'))
        i++;

    size_t cmdStart = i;
    while (i < line.size() && line[i] != ' ' && line[i] != '\t')
        i++;
    cmd = line.substr(cmdStart, i - cmdStart);

    while (i < line.size())
    {
        while (i < line.size() && (line[i] == ' ' || line[i] == '\t'))
            i++;
        if (i == line.size())
            break;

        if (line[i] == ':')
        {
            args.push_back(line.substr(i + 1));
            break;
        }
        else
        {
            size_t argStart = i;
            while (i < line.size() && line[i] != ' ' && line[i] != '\t')
                i++;
            args.push_back(line.substr(argStart, i - argStart));
        }
    }

    executeCommand(sockFd, cmd, args);
}