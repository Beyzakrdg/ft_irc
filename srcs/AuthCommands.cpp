#include "../includes/Server.hpp"
#include <cctype>

// Converts cmd to uppercase and dispatches to the appropriate handler.
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
    else if (cmd == "PONG")
        cmdPong(sockFd, client, args);
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
        if (it->second.getdisplayNick() == nick)
            return &(it->second);
    }
    return NULL;
}

Channel* Server::getChannelByName(std::string name)
{
    if (channels.find(name) != channels.end())
        return &channels.at(name);
    return NULL;
}

// --- Authentication commands ---

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
    if (args[0] == psswd)
    {
        client.setHasPassword(true);

    } else {
        std::string msg = ":server 464 * :Password incorrect\r\n";
        sendMessage(sockFd, msg);
    }
}

void Server::cmdNick(int sockFd, Client &client, std::vector<std::string> args)
{
    if (!client.getHasPassword())
        return;
    if (args.empty())
    {
        std::string nick = client.getdisplayNick().empty() ? "*" : client.getdisplayNick();
        std::string msg = ":server 431 " + nick + " :No nickname given\r\n";
        sendMessage(sockFd, msg);
        return;
    }

    std::string newNick = args[0];

    // Nickname validation: must start with a letter or special char, not digit or '-'
    if (newNick.empty() || newNick.length() > 9)
    {
        std::string nick = client.getdisplayNick().empty() ? "*" : client.getdisplayNick();
        std::string msg = ":server 432 " + nick + " " + newNick + " :Erroneous nickname\r\n";
        sendMessage(sockFd, msg);
        return;
    }
    // First char must be letter or special: [\]^_`{|}
    char first = newNick[0];
    if (!std::isalpha(first) && first != '[' && first != '\\' && first != ']'
        && first != '^' && first != '_' && first != '`'
        && first != '{' && first != '|' && first != '}')
    {
        std::string nick = client.getdisplayNick().empty() ? "*" : client.getdisplayNick();
        std::string msg = ":server 432 " + nick + " " + newNick + " :Erroneous nickname\r\n";
        sendMessage(sockFd, msg);
        return;
    }
    // Remaining chars: letter, digit, special, or -
    for (size_t i = 1; i < newNick.length(); i++)
    {
        char c = newNick[i];
        if (!std::isalnum(c) && c != '-' && c != '[' && c != '\\' && c != ']'
            && c != '^' && c != '_' && c != '`'
            && c != '{' && c != '|' && c != '}')
        {
            std::string nick = client.getdisplayNick().empty() ? "*" : client.getdisplayNick();
            std::string msg = ":server 432 " + nick + " " + newNick + " :Erroneous nickname\r\n";
            sendMessage(sockFd, msg);
            return;
        }
    }

    if (getClientByNick(newNick) != NULL && getClientByNick(newNick)->getFd() != sockFd)
    {
        std::string nick = client.getdisplayNick().empty() ? "*" : client.getdisplayNick();
        std::string msg = ":server 433 " + nick + " " + newNick + " :Nickname is already in use\r\n";
        sendMessage(sockFd, msg);
        return;
    }

    std::string oldNick = client.getdisplayNick();
    client.setdisplayNick(newNick);

    if (!oldNick.empty() && client.getsuccesLogin())
    {
        // NICK mesajı için eski prefix'i elle oluşturuyoruz (nick değişmeden önceki hali)
        std::string oldPrefix = oldNick + "!" + (client.getuserName().empty() ? "unknown" : client.getuserName()) + "@" + client.getHostname();
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

    if (!client.getsuccesLogin() && client.isRegistered())
    {
        client.setsuccessLogin(true);
        std::string welcome = ":server 001 " + client.getdisplayNick() + " :Welcome to the ft_irc network " + client.getdisplayNick() + "\r\n";
        sendMessage(sockFd, welcome);

    }
}

void Server::cmdUser(int sockFd, Client &client, std::vector<std::string> args)
{
    if (!client.getHasPassword())
        return;
    if (client.getsuccesLogin())
    {
        std::string msg = ":server 462 " + client.getdisplayNick() + " :You may not reregister\r\n";
        sendMessage(sockFd, msg);
        return;
    }
    if (args.size() < 4)
    {
        std::string nick = client.getdisplayNick().empty() ? "*" : client.getdisplayNick();
        std::string msg = ":server 461 " + nick + " USER :Not enough parameters\r\n";
        sendMessage(sockFd, msg);
        return;
    }
    client.setuserName(args[0]);

    if (!client.getsuccesLogin() && client.isRegistered())
    {
        client.setsuccessLogin(true);
        std::string welcome = ":server 001 " + client.getdisplayNick() + " :Welcome to the ft_irc network " + client.getdisplayNick() + "\r\n";
        sendMessage(sockFd, welcome);

    }
}
