#include "../includes/Server.hpp"
#include <cstdlib>
#include <cstdio>

void Server::executeCommand(int sockFd, std::string cmd, std::vector<std::string> args)
{
    if (cmd != "PING")
    {
        std::cout << "Komut: " << cmd << " (Client FD: " << sockFd << ")" << std::endl;
        for (size_t i = 0; i < args.size(); i++)
        {
            std::cout << " - Parametre " << i << ": " << args[i] << std::endl;
        }
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
    else if (cmd == "PING")
        cmdPing(sockFd, client, args);
    else if (cmd == "QUIT")
        cmdQuit(sockFd, client, args);
    else if (cmd == "PART")
        cmdPart(sockFd, client, args);
    else if (cmd == "CAP")
    {
        if (!args.empty() && args[0] == "LS")
            sendMessage(sockFd, "CAP * LS :\r\n");
        return;
    }
}

void Server::cmdPass(int sockFd, Client &client, std::vector<std::string> args)
{
    if (args.empty())
    {
        std::string msg = "461 PASS :Not enough parameters\r\n";
        sendMessage(sockFd, msg);
        return;
    }
    if (args[0] == psswd)
    {
        client.setHasPassword(true);
        std::cout << "Client " << sockFd << " password correct." << std::endl;
    } else {
        std::string msg = "464 :Password incorrect\r\n";
        sendMessage(sockFd, msg);
    }
}

void Server::cmdNick(int sockFd, Client &client, std::vector<std::string> args)
{
    if (!client.getHasPassword())
        return;
    if (args.empty())
    {
        std::string msg = "431 :No nickname given\r\n";
        sendMessage(sockFd, msg);
        return;
    }
    
    std::string newNick = args[0];
    if (getClientByNick(newNick) != NULL && getClientByNick(newNick)->getFd() != sockFd)
    {
        std::string msg = "433 " + newNick + " :Nickname is already in use\r\n";
        sendMessage(sockFd, msg);
        return;
    }
    
    std::string oldNick = client.getdisplayNick();
    client.setdisplayNick(newNick);
    
    if (!oldNick.empty() && client.getsuccesLogin())
    {
        std::string nickMsg = ":" + oldNick + " NICK :" + newNick + "\r\n";
        sendMessage(sockFd, nickMsg);
    }
    
    if (!client.getsuccesLogin() && client.isRegistered())
    {
        client.setsuccessLogin(true);
        std::string welcome = ":server 001 " + client.getdisplayNick() + " :Welcome to the ft_irc network " + client.getdisplayNick() + "\r\n";
        sendMessage(sockFd, welcome);
        std::cout << "Client " << sockFd << " successfully registered!" << std::endl;
    }
}

void Server::cmdUser(int sockFd, Client &client, std::vector<std::string> args)
{
    if (!client.getHasPassword())
        return;
    if (client.getsuccesLogin())
    {
        std::string msg = "462 :You may not reregister\r\n";
        sendMessage(sockFd, msg);
        return;
    }
    if (args.size() < 4)
    {
        std::string msg = "461 USER :Not enough parameters\r\n";
        sendMessage(sockFd, msg);
        return;
    }
    client.setuserName(args[0]);
    
    if (!client.getsuccesLogin() && client.isRegistered())
    {
        client.setsuccessLogin(true);
        std::string welcome = ":server 001 " + client.getdisplayNick() + " :Welcome to the ft_irc network " + client.getdisplayNick() + "\r\n";
        sendMessage(sockFd, welcome);
        std::cout << "Client " << sockFd << " successfully registered!" << std::endl;
    }
}

void Server::cmdJoin(int sockFd, Client &client, std::vector<std::string> args)
{
    if (!client.getsuccesLogin())
        return;
    if (args.empty())
    {
        std::string msg = "461 JOIN :Not enough parameters\r\n";
        sendMessage(sockFd, msg);
        return;
    }

    std::string channelName = args[0];
    std::string providedKey = (args.size() > 1) ? args[1] : "";
    if (channels.find(channelName) == channels.end())
    {
        channels.insert(std::make_pair(channelName, Channel(channelName)));
        channels.at(channelName).addOperator(&client);
    }
    
    Channel &chan = channels.at(channelName);
    if (chan.isInviteOnly() && !chan.isInvited(client.getdisplayNick()))
    {
        std::string msg = "473 " + channelName + " :Cannot join channel (+i)\r\n";
        sendMessage(sockFd, msg);
        return;
    }
    if (!chan.getKey().empty() && chan.getKey() != providedKey)
    {
        std::string msg = "475 " + channelName + " :Cannot join channel (+k)\r\n";
        sendMessage(sockFd, msg);
        return;
    }
    if (chan.getUserLimit() != -1 && (int)chan.getClientCount() >= chan.getUserLimit())
    {
        std::string msg = "471 " + channelName + " :Cannot join channel (+l)\r\n";
        sendMessage(sockFd, msg);
        return;
    }

    chan.addClient(&client);
    chan.removeInvite(client.getdisplayNick());
    std::string joinMsg = ":" + client.getdisplayNick() + " JOIN :" + channelName + "\r\n";
    chan.broadcastMessage(joinMsg, NULL);

    if (!chan.getTopic().empty())
    {
        std::string topicMsg = "332 " + client.getdisplayNick() + " " + channelName + " :" + chan.getTopic() + "\r\n";
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
        
    std::string namesMsg = "353 " + client.getdisplayNick() + " = " + channelName + " :" + namesList + "\r\n";
    sendMessage(sockFd, namesMsg);
    
    std::string endNamesMsg = "366 " + client.getdisplayNick() + " " + channelName + " :End of /NAMES list\r\n";
    sendMessage(sockFd, endNamesMsg);
}

void Server::cmdPrivmsg(int sockFd, Client &client, std::vector<std::string> args)
{
    if (!client.getsuccesLogin())
        return;
    if (args.size() < 2)
    {
        std::string msg = "412 :No text to send\r\n";
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
    }
}

void Server::cmdTopic(int sockFd, Client &client, std::vector<std::string> args)
{
    if (!client.getsuccesLogin())
        return;
    if (args.empty())
    {
        std::string msg = "461 TOPIC :Not enough parameters\r\n";
        sendMessage(sockFd, msg);
        return;
    }

    std::string channelName = args[0];

    if (channels.find(channelName) == channels.end())
    {
        std::string msg = "403 " + channelName + " :No such channel\r\n";
        sendMessage(sockFd, msg);
        return;
    }

    Channel &chan = channels.at(channelName);

    if (!chan.isClientInChannel(&client))
    {
        std::string msg = "442 " + channelName + " :You're not on that channel\r\n";
        sendMessage(sockFd, msg);
        return;
    }

    if (args.size() == 1)
    {
        std::string currentTopic = chan.getTopic();
        if (currentTopic.empty())
        {
            std::string msg = "331 " + client.getdisplayNick() + " " + channelName + " :No topic is set\r\n";
            sendMessage(sockFd, msg);
        } 
        else
        {
            std::string msg = "332 " + client.getdisplayNick() + " " + channelName + " :" + currentTopic + "\r\n";
            sendMessage(sockFd, msg);
        }
    } 
    else
    {
        if (chan.isTopicRestricted() && !chan.isOperator(&client))
        {
             std::string msg = "482 " + channelName + " :You're not channel operator\r\n";
             sendMessage(sockFd, msg);
             return;
        }

        std::string newTopic = args[1];
        chan.setTopic(newTopic);

        std::string topicMsg = ":" + client.getdisplayNick() + " TOPIC " + channelName + " :" + newTopic + "\r\n";
        chan.broadcastMessage(topicMsg, NULL);
    }
}

void Server::cmdKick(int sockFd, Client &client, std::vector<std::string> args)
{
    if (!client.getsuccesLogin())
        return;
    if (args.size() < 2)
    {
        std::string msg = "461 KICK :Not enough parameters\r\n";
        sendMessage(sockFd, msg);
        return;
    }
    std::string channelName = args[0];
    std::string targetNick = args[1];
    std::string reason = (args.size() > 2) ? args[2] : "No reason given";

    if (channels.find(channelName) == channels.end())
    {
        std::string msg = "403 " + channelName + " :No such channel\r\n";
        sendMessage(sockFd, msg);
        return;
    }

    Channel &chan = channels.at(channelName);

    if (!chan.isClientInChannel(&client))
    {
        std::string msg = "442 " + channelName + " :You're not on that channel\r\n";
        sendMessage(sockFd, msg);
        return;
    }

    if (!chan.isOperator(&client))
    {
         std::string msg = "482 " + channelName + " :You're not channel operator\r\n";
         sendMessage(sockFd, msg);
         return;
    }
    Client* targetClient = getClientByNick(targetNick);

    if (!targetClient)
    {
        std::string msg = "401 " + targetNick + " :No such nick/channel\r\n";
        sendMessage(sockFd, msg);
        return;
    }

    if (!chan.isClientInChannel(targetClient))
    {
        std::string msg = "441 " + targetNick + " " + channelName + " :They aren't on that channel\r\n";
        sendMessage(sockFd, msg);
        return;
    }
    std::string kickMsg = ":" + client.getdisplayNick() + " KICK " + channelName + " " + targetNick + " :" + reason + "\r\n";
    chan.broadcastMessage(kickMsg, NULL);

    chan.removeClient(targetClient);
    if (chan.getClientCount() == 0)
        channels.erase(channelName);
}

void Server::cmdInvite(int sockFd, Client &client, std::vector<std::string> args)
{
    if (!client.getsuccesLogin())
        return;
    if (args.size() < 2)
    {
        std::string msg = "461 INVITE :Not enough parameters\r\n";
        sendMessage(sockFd, msg);
        return;
    }

    std::string targetNick = args[0];
    std::string channelName = args[1];

    if (channels.find(channelName) == channels.end())
    {
        std::string msg = "403 " + channelName + " :No such channel\r\n";
        sendMessage(sockFd, msg);
        return;
    }

    Channel &chan = channels.at(channelName);

    if (!chan.isClientInChannel(&client))
    {
        std::string msg = "442 " + channelName + " :You're not on that channel\r\n";
        sendMessage(sockFd, msg);
        return;
    }
    Client* targetClient = getClientByNick(targetNick);

    if (!targetClient)
    {
        std::string msg = "401 " + targetNick + " :No such nick/channel\r\n";
        sendMessage(sockFd, msg);
        return;
    }

    if (chan.isClientInChannel(targetClient))
    {
        std::string msg = "443 " + targetNick + " " + channelName + " :is already on channel\r\n";
        sendMessage(sockFd, msg);
        return;
    }
    chan.addInvite(targetNick);
    std::string replyMsg = "341 " + client.getdisplayNick() + " " + targetNick + " " + channelName + "\r\n";
    sendMessage(sockFd, replyMsg);
    std::string inviteMsg = ":" + client.getdisplayNick() + " INVITE " + targetNick + " :" + channelName + "\r\n";
    sendMessage(targetClient->getFd(), inviteMsg);
}

void Server::sendMessage(int fd, std::string msg)
{
    send(fd, msg.c_str(), msg.length(), 0);
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

void Server::cmdMode(int sockFd, Client &client, std::vector<std::string> args)
{
    if (!client.getsuccesLogin())
        return;
    if (args.empty())
    {
        std::string msg = "461 MODE :Not enough parameters\r\n";
        sendMessage(sockFd, msg);
        return;
    }

    std::string target = args[0];
    if (target[0] != '#')
        return;

    if (channels.find(target) == channels.end())
    {
        std::string msg = "403 " + target + " :No such channel\r\n";
        sendMessage(sockFd, msg);
        return;
    }

    Channel &chan = channels.at(target);
    if (args.size() == 1)
    {
        std::string modes = "+";
        if
            (chan.isInviteOnly()) modes += "i";
        if
            (chan.isTopicRestricted()) modes += "t";
        if
            (!chan.getKey().empty()) modes += "k";
        if
            (chan.getUserLimit() != -1) modes += "l";
        std::string msg = "324 " + client.getdisplayNick() + " " + target + " " + modes + "\r\n";
        sendMessage(sockFd, msg);
        return;
    }
    if (!chan.isOperator(&client))
    {
        std::string msg = "482 " + target + " :You're not channel operator\r\n";
        sendMessage(sockFd, msg);
        return;
    }

    std::string modeStr = args[1];
    bool adding = true;
    size_t argIndex = 2;

    for (size_t i = 0; i < modeStr.length(); ++i)
    {
        char m = modeStr[i];
        if (m == '+')
            adding = true;
        else if (m == '-')
            adding = false;
        else if (m == 'i')
        {
            chan.setInviteOnly(adding);
            std::string broadcast = ":" + client.getdisplayNick() + " MODE " + target + (adding ? " +i\r\n" : " -i\r\n");
            chan.broadcastMessage(broadcast, NULL);
        }
        else if (m == 't')
        {
            chan.setTopicRestricted(adding);
            std::string broadcast = ":" + client.getdisplayNick() + " MODE " + target + (adding ? " +t\r\n" : " -t\r\n");
            chan.broadcastMessage(broadcast, NULL);
        }
        else if (m == 'k')
        {
            if (adding)
            {
                if (argIndex < args.size())
                {
                    chan.setKey(args[argIndex++]);
                    std::string broadcast = ":" + client.getdisplayNick() + " MODE " + target + " +k " + chan.getKey() + "\r\n";
                    chan.broadcastMessage(broadcast, NULL);
                }
            }
            else
            {
                chan.setKey("");
                std::string broadcast = ":" + client.getdisplayNick() + " MODE " + target + " -k\r\n";
                chan.broadcastMessage(broadcast, NULL);
            }
        } 
        else if (m == 'o')
        {
            if (argIndex < args.size())
            {
                std::string targetNick = args[argIndex++];
                Client* targetClient = getClientByNick(targetNick);
                if (targetClient && chan.isClientInChannel(targetClient))
                {
                    if (adding)
                    {
                        chan.addOperator(targetClient);
                        std::string broadcast = ":" + client.getdisplayNick() + " MODE " + target + " +o " + targetNick + "\r\n";
                        chan.broadcastMessage(broadcast, NULL);
                    }
                    else
                    {
                        chan.removeOperator(targetClient);
                        std::string broadcast = ":" + client.getdisplayNick() + " MODE " + target + " -o " + targetNick + "\r\n";
                        chan.broadcastMessage(broadcast, NULL);
                    }
                }
            }
        }
        else if (m == 'l')
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
                        std::string broadcast = ":" + client.getdisplayNick() + " MODE " + target + " +l " + buf + "\r\n";
                        chan.broadcastMessage(broadcast, NULL);
                    }
                }
            }
            else
            {
                chan.setUserLimit(-1);
                std::string broadcast = ":" + client.getdisplayNick() + " MODE " + target + " -l\r\n";
                chan.broadcastMessage(broadcast, NULL);
            }
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
    
    for (size_t i = 0; i < fds.size(); i++)
    {
        if (fds[i].fd == sockFd)
        {
            fds.erase(fds.begin() + i);
            break;
        }
    }
}

void Server::cmdPart(int sockFd, Client &client, std::vector<std::string> args)
{
    if (!client.getsuccesLogin())
        return;
        
    if (args.empty())
    {
        std::string msg = "461 PART :Not enough parameters\r\n";
        sendMessage(sockFd, msg);
        return;
    }
    
    std::string channelName = args[0];
    std::string reason = (args.size() > 1) ? args[1] : "";
    
    if (channels.find(channelName) == channels.end())
    {
        std::string msg = "403 " + channelName + " :No such channel\r\n";
        sendMessage(sockFd, msg);
        return;
    }
    
    Channel &chan = channels.at(channelName);
    if (!chan.isClientInChannel(&client))
    {
        std::string msg = "442 " + channelName + " :You're not on that channel\r\n";
        sendMessage(sockFd, msg);
        return;
    }
    
    std::string partMsg = ":" + client.getdisplayNick() + " PART " + channelName;
    if (!reason.empty())
    {
        partMsg += " :" + reason;
    }
    partMsg += "\r\n";
    
    chan.broadcastMessage(partMsg, NULL);
    chan.removeClient(&client);
    if (chan.getClientCount() == 0)
        channels.erase(channelName);
}
