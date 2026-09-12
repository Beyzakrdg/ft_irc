#include "../includes/Server.hpp"

volatile sig_atomic_t Server::_running = 1;

void Server::signalHandler(int signum)
{
    (void)signum;
    Server::_running = 0;
}

Server::Server(int port, std::string password) : portNo(port), serverFd(-1), psswd(password)
{
}

Server::~Server()
{
    shutdown();
}

void Server::shutdown()
{

    for (std::map<int, Client>::iterator it = clients.begin(); it != clients.end(); ++it)
    {
        std::string msg = ":server NOTICE * :Server shutting down\r\n";
        send(it->first, msg.c_str(), msg.length(), 0);
        close(it->first);
    }
    clients.clear();
    channels.clear();
    clientBuff.clear();
    outBuffers.clear();
    fds.clear();
    if (serverFd != -1)
    {
        close(serverFd);
        serverFd = -1;
    }
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

    struct sigaction sa;
    sa.sa_handler = Server::signalHandler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);
    sigaction(SIGQUIT, &sa, NULL);
}

void Server::run()
{
    time_t lastPingCheck = time(NULL);

    while (_running)
    {
        int pollResult = poll(&fds[0], fds.size(), 500);
        if (pollResult < 0)
        {
            if (!_running)
                break;
            throw std::runtime_error("Poll failed");
        }

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
                        continue;
                    }
                }
            }
            if (fds[i].fd != serverFd && (fds[i].revents & POLLOUT))
            {
                flushOutBuffer(fds[i].fd);
            }
        }

        // Her saniye ping/timeout kontrolu yap
        time_t now = time(NULL);
        if (now - lastPingCheck >= 1)
        {
            lastPingCheck = now;
            checkPingTimeouts();
        }
    }
    std::cout << "Server stopped." << std::endl;
}

void Server::checkPingTimeouts()
{
    time_t now = time(NULL);
    std::vector<int> toDisconnect;

    for (std::map<int, Client>::iterator it = clients.begin(); it != clients.end(); ++it)
    {
        Client &client = it->second;

        // Henuz login olmamis clientlari atla
        if (!client.getsuccesLogin())
            continue;

        time_t lastPong     = client.getLastPong();
        time_t lastPingSent = client.getLastPingSent();

        // PING_TIMEOUT suresi doldu ve biz PONG bekliyorduk -> baglantıyı kes
        if (lastPingSent != 0 && (now - lastPingSent) >= PING_TIMEOUT)
        {
            toDisconnect.push_back(it->first);
            continue;
        }

        // Henuz PING gondermemissek ya da bir onceki PONG geldikten beri
        // PING_INTERVAL gecti -> yeni PING gonder
        if (lastPingSent == 0 && (now - lastPong) >= PING_INTERVAL)
        {
            std::string pingMsg = "PING :server\r\n";
            sendMessage(it->first, pingMsg);
            client.setLastPingSent(now);
        }
    }

    // Zaman asimina ugrayan clientlari baglantidan at
    for (size_t i = 0; i < toDisconnect.size(); ++i)
    {
        std::map<int, Client>::iterator it = clients.find(toDisconnect[i]);
        if (it != clients.end())
        {
            // Tum kanallara QUIT bildir
            std::string quitMsg = ":" + it->second.getPrefix() + " QUIT :Ping timeout\r\n";
            for (std::map<std::string, Channel>::iterator chanIt = channels.begin();
                 chanIt != channels.end(); ++chanIt)
            {
                if (chanIt->second.isClientInChannel(&it->second))
                    chanIt->second.broadcastMessage(quitMsg, &it->second, outBuffers, fds);
            }
        }
        disconnectClient(toDisconnect[i]);
        // disconnectClient fd'yi fds listesinden siler, biz burada sadece clients'i temizledik
        for (size_t j = 0; j < fds.size(); ++j)
        {
            if (fds[j].fd == toDisconnect[i])
            {
                fds.erase(fds.begin() + j);
                break;
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
        disconnectClient(sockFd);
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
        if (clients.find(sockFd) == clients.end())
            return false;
        clientBuff[sockFd].erase(0, pos + 1);
    }
    return true;
}

void Server::flushOutBuffer(int sockFd)
{
    std::map<int, std::string>::iterator it = outBuffers.find(sockFd);
    if (it == outBuffers.end() || it->second.empty())
        return;
    int sent = send(sockFd, it->second.c_str(), it->second.length(), 0);
    if (sent > 0)
    {
        it->second.erase(0, sent);
        if (it->second.empty())
        {
            outBuffers.erase(it);
            updatePollEvents(sockFd, POLLIN);
        }
    }
    else if (sent <= 0)
    {
        disconnectClient(sockFd);
    }
}

void Server::updatePollEvents(int fd, short events)
{
    for (size_t i = 0; i < fds.size(); i++)
    {
        if (fds[i].fd == fd)
        {
            fds[i].events = events;
            return;
        }
    }
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
    clientPollFd.revents = 0;
    fds.push_back(clientPollFd);

    clients.insert(std::make_pair(sockFd, Client(sockFd)));

    // Store client's IP address as hostname
    char hostBuf[INET_ADDRSTRLEN];
    if (inet_ntop(AF_INET, &clientAddr.sin_addr, hostBuf, sizeof(hostBuf)) != NULL)
        clients.at(sockFd).setHostname(std::string(hostBuf));
    else
        clients.at(sockFd).setHostname("localhost");
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

void Server::disconnectClient(int sockFd)
{

    std::map<int, Client>::iterator it = clients.find(sockFd);
    if (it != clients.end())
    {
        Client* clientPtr = &(it->second);
        std::string quitMsg = ":" + clientPtr->getPrefix() + " QUIT :Connection lost\r\n";
        std::map<std::string, Channel>::iterator chanIt = channels.begin();
        while (chanIt != channels.end())
        {
            if (chanIt->second.isClientInChannel(clientPtr))
            {
                chanIt->second.broadcastMessage(quitMsg, clientPtr, outBuffers, fds);
                chanIt->second.removeClient(clientPtr);
            }
            
            if (chanIt->second.getClientCount() == 0)
            {
                std::map<std::string, Channel>::iterator toErase = chanIt;
                ++chanIt;
                channels.erase(toErase);
            }
            else
            {
                ++chanIt;
            }
        }
    }
    close(sockFd);
    clientBuff.erase(sockFd);
    outBuffers.erase(sockFd);
    clients.erase(sockFd);
}