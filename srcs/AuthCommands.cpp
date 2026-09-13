#include "../includes/Server.hpp"

static std::string getNickOrStar(const Client &client)
{
    if (client.getdisplayNick().empty())
        return "*";
    return client.getdisplayNick();
}

void Server::executeCommand(int sockFd, std::string cmd, std::vector<std::string> args)
{
    for (size_t i = 0; i < cmd.length(); i++)
    {
        cmd[i] = std::toupper(cmd[i]);
    }

    std::map<int, Client>::iterator it = clients.find(sockFd);
    if (it == clients.end())
        return;
    Client &client = it->second;

    if (cmd == "PASS")
        cmdPass(sockFd, client, args);
    else if (cmd == "NICK")
        cmdNick(sockFd, client, args);
    else if (cmd == "USER")
        cmdUser(sockFd, client, args);
    else if (cmd == "JOIN")
        cmdJoin(sockFd, client, args);
    else if (cmd == "MODE")
        cmdMode(sockFd, client, args);
    else if (cmd == "TOPIC")
        cmdTopic(sockFd, client, args);
    else if (cmd == "KICK")
        cmdKick(sockFd, client, args);
    else if (cmd == "INVITE")
        cmdInvite(sockFd, client, args);
    else if (cmd == "PRIVMSG")
        cmdPrivmsg(sockFd, client, args);
    else if (cmd == "NOTICE")
        cmdNotice(sockFd, client, args);
    else if (cmd == "PING")
        cmdPing(sockFd, client, args);
    else if (cmd == "QUIT")
        cmdQuit(sockFd, client, args);
    else if (cmd == "PART")
        cmdPart(sockFd, client, args);
    else if (cmd == "CAP")
    {
        if (!args.empty() && args[0] == "LS")
            sendMessage(sockFd, ":server CAP * LS :\r\n");
        return;
    }
}

void Server::sendMessage(int fd, std::string msg)
{
    outBuffers[fd] += msg;
    updatePollEvents(fd, POLLIN | POLLOUT);
}

Client* Server::getClientByNick(std::string nick)
{
    for (std::map<int, Client>::iterator it = clients.begin(); it != clients.end(); ++it)
    {
        if (Client::nickEquals(it->second.getdisplayNick(), nick))
            return &(it->second);
    }
    return NULL;
}

Channel* Server::getChannelByName(std::string name)
{
    std::map<std::string, Channel>::iterator it = channels.find(Client::ircLower(name));
    if (it != channels.end())
        return &(it->second);
    return NULL;
}

void Server::cmdPass(int sockFd, Client &client, std::vector<std::string> args)
{
    if (client.getsuccesLogin())
    {
        std::string msg = ":server 462 " + client.getdisplayNick() + " :You may not reregister\r\n";
        sendMessage(sockFd, msg);
        return;
    }
    if (args.empty())
    {
        std::string msg = ":server 461 * PASS :Not enough parameters\r\n";
        sendMessage(sockFd, msg);
        return;
    }
    if (args[0] != psswd)
    {
        client.setHasPassword(false);
        std::string msg = ":server 464 * :Password incorrect\r\n";
        sendMessage(sockFd, msg);
        return;
    }
    client.setHasPassword(true);
}

void Server::cmdNick(int sockFd, Client &client, std::vector<std::string> args)
{
    if (!client.getsuccesLogin() && !client.getHasPassword())
    {
        std::string msg = ":server 451 * :You have not registered, send PASS first\r\n";
        sendMessage(sockFd, msg);
        return;
    }
    client.setNickSent(true);
    if (args.empty())
    {
        std::string msg = ":server 431 " + getNickOrStar(client) + " :No nickname given\r\n";
        sendMessage(sockFd, msg);
        return;
    }

    std::string newNick = args[0];

    if (newNick.empty() || newNick.length() > 9)
    {
        std::string msg = ":server 432 " + getNickOrStar(client) + " " + newNick + " :Erroneous nickname\r\n";
        sendMessage(sockFd, msg);
        return;
    }

    char first = newNick[0];
    if (!std::isalpha(first) && first != '[' && first != '\\' && first != ']'
        && first != '^' && first != '_' && first != '`'
        && first != '{' && first != '|' && first != '}')
    {
        std::string msg = ":server 432 " + getNickOrStar(client) + " " + newNick + " :Erroneous nickname\r\n";
        sendMessage(sockFd, msg);
        return;
    }
    for (size_t i = 1; i < newNick.length(); i++)
    {
        char c = newNick[i];
        if (!std::isalnum(c) && c != '-' && c != '[' && c != '\\' && c != ']'
            && c != '^' && c != '_' && c != '`'
            && c != '{' && c != '|' && c != '}')
        {
            std::string msg = ":server 432 " + getNickOrStar(client) + " " + newNick + " :Erroneous nickname\r\n";
            sendMessage(sockFd, msg);
            return;
        }
    }

    if (getClientByNick(newNick) != NULL && getClientByNick(newNick)->getFd() != sockFd)
    {
        std::string msg = ":server 433 " + getNickOrStar(client) + " " + newNick + " :Nickname is already in use\r\n";
        sendMessage(sockFd, msg);
        return;
    }

    std::string oldNick = client.getdisplayNick();
    client.setdisplayNick(newNick);

    if (!oldNick.empty() && client.getsuccesLogin())
    {
        std::string userName;
        if (client.getuserName().empty())
            userName = "unknown";
        else
            userName = client.getuserName();
        std::string oldPrefix = oldNick + "!" + userName + "@" + client.getHostname();
        std::string nickMsg = ":" + oldPrefix + " NICK :" + newNick + "\r\n";
        sendMessage(sockFd, nickMsg);

        std::vector<int> targetFds;
        for (std::map<int, Client>::iterator clIt = clients.begin(); clIt != clients.end(); ++clIt)
        {
            int peerFd = clIt->first;
            if (peerFd == sockFd)
                continue;
            for (std::map<std::string, Channel>::iterator chanIt = channels.begin(); chanIt != channels.end(); ++chanIt)
            {
                if (chanIt->second.isClientInChannel(&client) && chanIt->second.isClientInChannel(&(clIt->second)))
                {
                    targetFds.push_back(peerFd);
                    break;
                }
            }
        }
        for (size_t i = 0; i < targetFds.size(); ++i)
        {
            sendMessage(targetFds[i], nickMsg);
        }
    }

    if (!client.getsuccesLogin() && !client.getuserName().empty())
    {
        client.setsuccessLogin(true);
        std::string welcome = ":server 001 " + client.getdisplayNick() + " :Welcome to the ft_irc network " + client.getdisplayNick() + "\r\n";
        sendMessage(sockFd, welcome);
    }
}

void Server::cmdUser(int sockFd, Client &client, std::vector<std::string> args)
{
    if (client.getsuccesLogin())
    {
        std::string msg = ":server 462 " + client.getdisplayNick() + " :You may not reregister\r\n";
        sendMessage(sockFd, msg);
        return;
    }
    if (!client.getHasPassword())
    {
        std::string msg = ":server 451 * :You have not registered, send PASS first\r\n";
        sendMessage(sockFd, msg);
        return;
    }
    if (!client.getNickSent())
    {
        std::string msg = ":server 451 * :You have not registered, send NICK first\r\n";
        sendMessage(sockFd, msg);
        return;
    }
    if (args.size() < 4)
    {
        std::string msg = ":server 461 " + getNickOrStar(client) + " USER :Not enough parameters\r\n";
        sendMessage(sockFd, msg);
        return;
    }
    client.setuserName(args[0]);
    if (client.getdisplayNick().empty())
        return;
    client.setsuccessLogin(true);
    std::string welcome = ":server 001 " + client.getdisplayNick() + " :Welcome to the ft_irc network " + client.getdisplayNick() + "\r\n";
    sendMessage(sockFd, welcome);
}
