#include "../includes/Server.hpp"

// --- Messaging and connection commands ---

void Server::cmdPrivmsg(int sockFd, Client &client, std::vector<std::string> args)
{
    if (!client.getsuccesLogin())
        return;
    if (args.size() < 2)
    {
        std::string msg = "412 " + client.getdisplayNick() + " :No text to send\r\n";
        sendMessage(sockFd, msg);
        return;
    }

    std::string target = args[0];
    std::string message = args[1];

    if (target[0] == '#')
    {
        if (channels.find(target) != channels.end())
        {
            Channel &chan = channels.at(target);
            if (chan.isClientInChannel(&client))
            {
                std::string privMsg = ":" + client.getdisplayNick() + " PRIVMSG " + target + " :" + message + "\r\n";
                chan.broadcastMessage(privMsg, &client);
            }
            else
            {
                std::string msg = "404 " + client.getdisplayNick() + " " + target + " :Cannot send to channel\r\n";
                sendMessage(sockFd, msg);
            }
        }
        else
        {
            std::string msg = "401 " + client.getdisplayNick() + " " + target + " :No such nick/channel\r\n";
            sendMessage(sockFd, msg);
        }
    }
    else
    {
        Client* targetClient = getClientByNick(target);
        if (targetClient)
        {
            std::string privMsg = ":" + client.getdisplayNick() + " PRIVMSG " + target + " :" + message + "\r\n";
            sendMessage(targetClient->getFd(), privMsg);
        }
        else
        {
            std::string msg = "401 " + client.getdisplayNick() + " " + target + " :No such nick/channel\r\n";
            sendMessage(sockFd, msg);
        }
    }
}

void Server::cmdPing(int sockFd, Client &client, std::vector<std::string> args)
{
    (void)client;
    if (args.empty())
    {
        std::string msg = "409 :No origin specified\r\n";
        sendMessage(sockFd, msg);
        return;
    }
    std::string msg = "PONG :" + args[0] + "\r\n";
    sendMessage(sockFd, msg);
}

void Server::cmdQuit(int sockFd, Client &client, std::vector<std::string> args)
{
    std::string reason = "Client Quit";
    if (!args.empty())
        reason = args[0];

    std::string quitMsg = ":" + client.getdisplayNick() + " QUIT :" + reason + "\r\n";

    std::map<std::string, Channel>::iterator chanIt = channels.begin();
    while (chanIt != channels.end())
    {
        if (chanIt->second.isClientInChannel(&client))
            chanIt->second.broadcastMessage(quitMsg, &client);
        ++chanIt;
    }

    disconnectClient(sockFd);
}
