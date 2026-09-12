#include "../includes/Server.hpp"

void Server::cmdPrivmsg(int sockFd, Client &client, std::vector<std::string> args)
{
    if (!client.getsuccesLogin())
        return;
    if (args.empty())
    {
        std::string msg = ":server 411 " + client.getdisplayNick() + " :No recipient given (PRIVMSG)\r\n";
        sendMessage(sockFd, msg);
        return;
    }
    if (args.size() < 2)
    {
        std::string msg = ":server 412 " + client.getdisplayNick() + " :No text to send\r\n";
        sendMessage(sockFd, msg);
        return;
    }

    std::string target = args[0];
    std::string message = args[1];

    if (target[0] == '#')
    {
        std::map<std::string, Channel>::iterator chanIt = channels.find(target);
        if (chanIt != channels.end())
        {
            Channel &chan = chanIt->second;
            if (chan.isClientInChannel(&client))
            {
                std::string privMsg = ":" + client.getPrefix() + " PRIVMSG " + target + " :" + message + "\r\n";
                chan.broadcastMessage(privMsg, &client, outBuffers, fds);
            }
            else
            {
                std::string msg = ":server 404 " + client.getdisplayNick() + " " + target + " :Cannot send to channel\r\n";
                sendMessage(sockFd, msg);
            }
        }
        else
        {
            std::string msg = ":server 401 " + client.getdisplayNick() + " " + target + " :No such nick/channel\r\n";
            sendMessage(sockFd, msg);
        }
    }
    else
    {
        Client* targetClient = getClientByNick(target);
        if (targetClient)
        {
            std::string privMsg = ":" + client.getPrefix() + " PRIVMSG " + target + " :" + message + "\r\n";
            sendMessage(targetClient->getFd(), privMsg);
        }
        else
        {
            std::string msg = ":server 401 " + client.getdisplayNick() + " " + target + " :No such nick/channel\r\n";
            sendMessage(sockFd, msg);
        }
    }
}

void Server::cmdNotice(int sockFd, Client &client, std::vector<std::string> args)
{
    (void)sockFd;
    if (!client.getsuccesLogin() || args.size() < 2)
        return;

    std::string target = args[0];
    std::string message = args[1];

    if (target[0] == '#')
    {
        std::map<std::string, Channel>::iterator chanIt = channels.find(target);
        if (chanIt != channels.end())
        {
            Channel &chan = chanIt->second;
            if (chan.isClientInChannel(&client))
            {
                std::string noticeMsg = ":" + client.getPrefix() + " NOTICE " + target + " :" + message + "\r\n";
                chan.broadcastMessage(noticeMsg, &client, outBuffers, fds);
            }
        }
    }
    else
    {
        Client* targetClient = getClientByNick(target);
        if (targetClient)
        {
            std::string noticeMsg = ":" + client.getPrefix() + " NOTICE " + target + " :" + message + "\r\n";
            sendMessage(targetClient->getFd(), noticeMsg);
        }
    }
}

void Server::cmdPing(int sockFd, Client &client, std::vector<std::string> args)
{
    (void)client;
    if (args.empty())
    {
        std::string msg = ":server 409 * :No origin specified\r\n";
        sendMessage(sockFd, msg);
        return;
    }
    std::string msg = ":server PONG server :" + args[0] + "\r\n";
    sendMessage(sockFd, msg);
}

void Server::cmdPong(int sockFd, Client &client, std::vector<std::string> args)
{
    (void)sockFd;
    (void)client;
    (void)args;
}

void Server::cmdQuit(int sockFd, Client &client, std::vector<std::string> args)
{
    std::string reason = "Client Quit";
    if (!args.empty())
        reason = args[0];

    std::string quitMsg = ":" + client.getPrefix() + " QUIT :" + reason + "\r\n";

    std::map<std::string, Channel>::iterator chanIt = channels.begin();
    while (chanIt != channels.end())
    {
        if (chanIt->second.isClientInChannel(&client))
            chanIt->second.broadcastMessage(quitMsg, &client, outBuffers, fds);
        ++chanIt;
    }

    disconnectClient(sockFd);
}
