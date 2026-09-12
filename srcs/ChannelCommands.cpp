#include "../includes/Server.hpp"
#include <cstdlib>
#include <cstdio>

Channel* Server::getValidChannel(int sockFd, Client &client, const std::string &name)
{
    std::map<std::string, Channel>::iterator it = channels.find(name);
    if (it == channels.end())
    {
        std::string msg = ":server 403 " + client.getdisplayNick() + " " + name + " :No such channel\r\n";
        sendMessage(sockFd, msg);
        return NULL;
    }
    return &(it->second);
}

bool Server::checkInChannel(int sockFd, Client &client, Channel &chan, const std::string &name)
{
    if (!chan.isClientInChannel(&client))
    {
        std::string msg = ":server 442 " + client.getdisplayNick() + " " + name + " :You're not on that channel\r\n";
        sendMessage(sockFd, msg);
        return false;
    }
    return true;
}

bool Server::checkIsOperator(int sockFd, Client &client, Channel &chan, const std::string &name)
{
    if (!chan.isOperator(&client))
    {
        std::string msg = ":server 482 " + client.getdisplayNick() + " " + name + " :You're not channel operator\r\n";
        sendMessage(sockFd, msg);
        return false;
    }
    return true;
}

void Server::cmdJoin(int sockFd, Client &client, std::vector<std::string> args)
{
    if (!client.getsuccesLogin())
        return;
    if (args.empty())
    {
        std::string msg = ":server 461 " + client.getdisplayNick() + " JOIN :Not enough parameters\r\n";
        sendMessage(sockFd, msg);
        return;
    }

    std::string channelList = args[0];
    std::string keyList;
    if (args.size() > 1)
        keyList = args[1];
    std::vector<std::string> channelNames;
    std::vector<std::string> keys;

    size_t pos = 0;
    while ((pos = channelList.find(',')) != std::string::npos)
    {
        channelNames.push_back(channelList.substr(0, pos));
        channelList.erase(0, pos + 1);
    }
    channelNames.push_back(channelList);

    if (!keyList.empty())
    {
        pos = 0;
        while ((pos = keyList.find(',')) != std::string::npos)
        {
            keys.push_back(keyList.substr(0, pos));
            keyList.erase(0, pos + 1);
        }
        keys.push_back(keyList);
    }

    for (size_t ci = 0; ci < channelNames.size(); ++ci)
    {
        std::string channelName = channelNames[ci];
        std::string providedKey;
        if (ci < keys.size())
            providedKey = keys[ci];

        if (channelName.empty() || channelName[0] != '#')
        {
            std::string msg = ":server 476 " + client.getdisplayNick() + " " + channelName + " :Bad Channel Mask\r\n";
            sendMessage(sockFd, msg);
            continue;
        }

        std::map<std::string, Channel>::iterator chanIt = channels.find(channelName);
        if (chanIt == channels.end())
        {
            std::pair<std::map<std::string, Channel>::iterator, bool> ret =
                channels.insert(std::make_pair(channelName, Channel(channelName)));
            chanIt = ret.first;
            chanIt->second.addOperator(&client);
        }

        Channel &chan = chanIt->second;

        if (chan.isClientInChannel(&client))
            continue;
        if (chan.isInviteOnly() && !chan.isInvited(client.getdisplayNick()))
        {
            std::string msg = ":server 473 " + client.getdisplayNick() + " " + channelName + " :Cannot join channel (+i)\r\n";
            sendMessage(sockFd, msg);
            continue;
        }
        if (!chan.getKey().empty() && chan.getKey() != providedKey)
        {
            std::string msg = ":server 475 " + client.getdisplayNick() + " " + channelName + " :Cannot join channel (+k)\r\n";
            sendMessage(sockFd, msg);
            continue;
        }
        if (chan.getUserLimit() != -1 && (int)chan.getClientCount() >= chan.getUserLimit())
        {
            std::string msg = ":server 471 " + client.getdisplayNick() + " " + channelName + " :Cannot join channel (+l)\r\n";
            sendMessage(sockFd, msg);
            continue;
        }

        chan.addClient(&client);
        chan.removeInvite(client.getdisplayNick());
        std::string joinMsg = ":" + client.getPrefix() + " JOIN :" + channelName + "\r\n";
        chan.broadcastMessage(joinMsg, NULL, outBuffers, fds);

        if (!chan.getTopic().empty())
        {
            std::string topicMsg = ":server 332 " + client.getdisplayNick() + " " + channelName + " :" + chan.getTopic() + "\r\n";
            sendMessage(sockFd, topicMsg);
        }

        std::string namesList = "";
        for (std::map<int, Client>::iterator it = clients.begin(); it != clients.end(); ++it)
        {
            if (chan.isClientInChannel(&(it->second)))
            {
                if (chan.isOperator(&(it->second)))
                    namesList += "@" + it->second.getdisplayNick() + " ";
                else
                    namesList += it->second.getdisplayNick() + " ";
            }
        }
        if (!namesList.empty() && namesList[namesList.length() - 1] == ' ')
            namesList.erase(namesList.length() - 1);

        std::string namesMsg = ":server 353 " + client.getdisplayNick() + " = " + channelName + " :" + namesList + "\r\n";
        sendMessage(sockFd, namesMsg);
        std::string endNamesMsg = ":server 366 " + client.getdisplayNick() + " " + channelName + " :End of /NAMES list\r\n";
        sendMessage(sockFd, endNamesMsg);
    }
}

void Server::cmdPart(int sockFd, Client &client, std::vector<std::string> args)
{
    if (!client.getsuccesLogin())
        return;
    if (args.empty())
    {
        std::string msg = ":server 461 " + client.getdisplayNick() + " PART :Not enough parameters\r\n";
        sendMessage(sockFd, msg);
        return;
    }

    std::string channelName = args[0];
    std::string reason;
    if (args.size() > 1)
        reason = args[1];

    Channel *chan = getValidChannel(sockFd, client, channelName);
    if (!chan)
        return;
    if (!checkInChannel(sockFd, client, *chan, channelName))
        return;

    std::string partMsg = ":" + client.getPrefix() + " PART " + channelName;
    if (!reason.empty())
        partMsg += " :" + reason;
    partMsg += "\r\n";

    chan->broadcastMessage(partMsg, NULL, outBuffers, fds);
    chan->removeClient(&client);
    if (chan->getClientCount() == 0)
        channels.erase(channelName);
}

void Server::cmdTopic(int sockFd, Client &client, std::vector<std::string> args)
{
    if (!client.getsuccesLogin())
        return;
    if (args.empty())
    {
        std::string msg = ":server 461 " + client.getdisplayNick() + " TOPIC :Not enough parameters\r\n";
        sendMessage(sockFd, msg);
        return;
    }

    std::string channelName = args[0];

    Channel *chan = getValidChannel(sockFd, client, channelName);
    if (!chan)
        return;
    if (!checkInChannel(sockFd, client, *chan, channelName))
        return;

    if (args.size() == 1)
    {
        std::string currentTopic = chan->getTopic();
        if (currentTopic.empty())
        {
            std::string msg = ":server 331 " + client.getdisplayNick() + " " + channelName + " :No topic is set\r\n";
            sendMessage(sockFd, msg);
        }
        else
        {
            std::string msg = ":server 332 " + client.getdisplayNick() + " " + channelName + " :" + currentTopic + "\r\n";
            sendMessage(sockFd, msg);
        }
    }
    else
    {
        if (chan->isTopicRestricted() && !checkIsOperator(sockFd, client, *chan, channelName))
            return;
        std::string newTopic = args[1];
        chan->setTopic(newTopic);
        std::string topicMsg = ":" + client.getPrefix() + " TOPIC " + channelName + " :" + newTopic + "\r\n";
        chan->broadcastMessage(topicMsg, NULL, outBuffers, fds);
    }
}

void Server::cmdKick(int sockFd, Client &client, std::vector<std::string> args)
{
    if (!client.getsuccesLogin())
        return;
    if (args.size() < 2)
    {
        std::string msg = ":server 461 " + client.getdisplayNick() + " KICK :Not enough parameters\r\n";
        sendMessage(sockFd, msg);
        return;
    }

    std::string channelName = args[0];
    std::string targetNick  = args[1];
    std::string reason;
    if (args.size() > 2)
        reason = args[2];
    else
        reason = "No reason given";

    Channel *chan = getValidChannel(sockFd, client, channelName);
    if (!chan)
        return;
    if (!checkInChannel(sockFd, client, *chan, channelName))
        return;
    if (!checkIsOperator(sockFd, client, *chan, channelName))
        return;

    Client *targetClient = getClientByNick(targetNick);
    if (!targetClient)
    {
        std::string msg = ":server 401 " + client.getdisplayNick() + " " + targetNick + " :No such nick/channel\r\n";
        sendMessage(sockFd, msg);
        return;
    }
    if (!chan->isClientInChannel(targetClient))
    {
        std::string msg = ":server 441 " + client.getdisplayNick() + " " + targetNick + " " + channelName + " :They aren't on that channel\r\n";
        sendMessage(sockFd, msg);
        return;
    }

    std::string kickMsg = ":" + client.getPrefix() + " KICK " + channelName + " " + targetNick + " :" + reason + "\r\n";
    chan->broadcastMessage(kickMsg, NULL, outBuffers, fds);
    chan->removeClient(targetClient);
    if (chan->getClientCount() == 0)
        channels.erase(channelName);
}

void Server::cmdInvite(int sockFd, Client &client, std::vector<std::string> args)
{
    if (!client.getsuccesLogin())
        return;
    if (args.size() < 2)
    {
        std::string msg = ":server 461 " + client.getdisplayNick() + " INVITE :Not enough parameters\r\n";
        sendMessage(sockFd, msg);
        return;
    }

    std::string targetNick  = args[0];
    std::string channelName = args[1];

    Channel *chan = getValidChannel(sockFd, client, channelName);
    if (!chan)
        return;
    if (!checkInChannel(sockFd, client, *chan, channelName))
        return;
    if (chan->isInviteOnly() && !checkIsOperator(sockFd, client, *chan, channelName))
        return;

    Client *targetClient = getClientByNick(targetNick);
    if (!targetClient)
    {
        std::string msg = ":server 401 " + client.getdisplayNick() + " " + targetNick + " :No such nick/channel\r\n";
        sendMessage(sockFd, msg);
        return;
    }
    if (chan->isClientInChannel(targetClient))
    {
        std::string msg = ":server 443 " + client.getdisplayNick() + " " + targetNick + " " + channelName + " :is already on channel\r\n";
        sendMessage(sockFd, msg);
        return;
    }

    chan->addInvite(targetNick);
    std::string replyMsg = ":server 341 " + client.getdisplayNick() + " " + targetNick + " " + channelName + "\r\n";
    sendMessage(sockFd, replyMsg);
    std::string inviteMsg = ":" + client.getPrefix() + " INVITE " + targetNick + " :" + channelName + "\r\n";
    sendMessage(targetClient->getFd(), inviteMsg);
}

void Server::handleModeI(Client &client, Channel &chan, const std::string &target, bool adding)
{
    chan.setInviteOnly(adding);
    std::string suffix;
    if (adding)
        suffix = " +i\r\n";
    else
        suffix = " -i\r\n";
    std::string broadcast = ":" + client.getPrefix() + " MODE " + target + suffix;
    chan.broadcastMessage(broadcast, NULL, outBuffers, fds);
}

void Server::handleModeT(Client &client, Channel &chan, const std::string &target, bool adding)
{
    chan.setTopicRestricted(adding);
    std::string suffix;
    if (adding)
        suffix = " +t\r\n";
    else
        suffix = " -t\r\n";
    std::string broadcast = ":" + client.getPrefix() + " MODE " + target + suffix;
    chan.broadcastMessage(broadcast, NULL, outBuffers, fds);
}

void Server::handleModeK(Client &client, Channel &chan, const std::string &target, bool adding,
                          std::vector<std::string> &args, size_t &argIndex)
{
    if (adding)
    {
        if (argIndex < args.size())
        {
            chan.setKey(args[argIndex++]);
            std::string broadcast = ":" + client.getPrefix() + " MODE " + target + " +k " + chan.getKey() + "\r\n";
            chan.broadcastMessage(broadcast, NULL, outBuffers, fds);
        }
    }
    else
    {
        chan.setKey("");
        std::string broadcast = ":" + client.getPrefix() + " MODE " + target + " -k\r\n";
        chan.broadcastMessage(broadcast, NULL, outBuffers, fds);
    }
}

void Server::handleModeO(int sockFd, Client &client, Channel &chan, const std::string &target, bool adding,
                          std::vector<std::string> &args, size_t &argIndex)
{
    if (argIndex >= args.size())
        return;

    std::string targetNick   = args[argIndex++];
    Client      *targetClient = getClientByNick(targetNick);

    if (!targetClient)
    {
        std::string msg = ":server 401 " + client.getdisplayNick() + " " + targetNick + " :No such nick/channel\r\n";
        sendMessage(sockFd, msg);
        return;
    }
    if (!chan.isClientInChannel(targetClient))
    {
        std::string msg = ":server 441 " + client.getdisplayNick() + " " + targetNick + " " + target + " :They aren't on that channel\r\n";
        sendMessage(sockFd, msg);
        return;
    }

    if (adding)
    {
        chan.addOperator(targetClient);
        std::string broadcast = ":" + client.getPrefix() + " MODE " + target + " +o " + targetNick + "\r\n";
        chan.broadcastMessage(broadcast, NULL, outBuffers, fds);
    }
    else
    {
        chan.removeOperator(targetClient);
        std::string broadcast = ":" + client.getPrefix() + " MODE " + target + " -o " + targetNick + "\r\n";
        chan.broadcastMessage(broadcast, NULL, outBuffers, fds);
    }
}

void Server::handleModeL(Client &client, Channel &chan, const std::string &target, bool adding,
                          std::vector<std::string> &args, size_t &argIndex)
{
    if (adding)
    {
        if (argIndex < args.size())
        {
            int limit = std::atoi(args[argIndex++].c_str());
            if (limit > 0)
            {
                chan.setUserLimit(limit);
                char buf[32];
                snprintf(buf, sizeof(buf), "%d", limit);
                std::string broadcast = ":" + client.getPrefix() + " MODE " + target + " +l " + buf + "\r\n";
                chan.broadcastMessage(broadcast, NULL, outBuffers, fds);
            }
        }
    }
    else
    {
        chan.setUserLimit(-1);
        std::string broadcast = ":" + client.getPrefix() + " MODE " + target + " -l\r\n";
        chan.broadcastMessage(broadcast, NULL, outBuffers, fds);
    }
}

void Server::cmdMode(int sockFd, Client &client, std::vector<std::string> args)
{
    if (!client.getsuccesLogin())
        return;
    if (args.empty())
    {
        std::string msg = ":server 461 " + client.getdisplayNick() + " MODE :Not enough parameters\r\n";
        sendMessage(sockFd, msg);
        return;
    }

    std::string target = args[0];
    if (target[0] != '#')
        return;

    Channel *chan = getValidChannel(sockFd, client, target);
    if (!chan)
        return;

    if (args.size() == 1)
    {
        std::string modes = "+";
        if (chan->isInviteOnly())    modes += "i";
        if (chan->isTopicRestricted()) modes += "t";
        if (!chan->getKey().empty()) modes += "k";
        if (chan->getUserLimit() != -1) modes += "l";
        std::string msg = ":server 324 " + client.getdisplayNick() + " " + target + " " + modes + "\r\n";
        sendMessage(sockFd, msg);
        return;
    }

    if (!checkIsOperator(sockFd, client, *chan, target))
        return;

    std::string modeStr = args[1];
    bool   adding   = true;
    size_t argIndex = 2;

    for (size_t i = 0; i < modeStr.length(); ++i)
    {
        char m = modeStr[i];
        if (m == '+')
            adding = true;
        else if (m == '-')
            adding = false;
        else if (m == 'i')
            handleModeI(client, *chan, target, adding);
        else if (m == 't')
            handleModeT(client, *chan, target, adding);
        else if (m == 'k')
            handleModeK(client, *chan, target, adding, args, argIndex);
        else if (m == 'o')
            handleModeO(sockFd, client, *chan, target, adding, args, argIndex);
        else if (m == 'l')
            handleModeL(client, *chan, target, adding, args, argIndex);
        else
        {
            std::string msg = ":server 472 " + client.getdisplayNick() + " " + std::string(1, m) + " :is unknown mode char to me\r\n";
            sendMessage(sockFd, msg);
        }
    }
}
