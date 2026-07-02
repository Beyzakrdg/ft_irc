#include "../includes/Channel.hpp"
#include <sys/socket.h>

Channel::Channel() : name(""), topic(""), inviteOnly(false), topicRestricted(false), key(""), userLimit(-1)
{}

Channel::Channel(std::string name) : name(name), topic(""), inviteOnly(false), topicRestricted(false), key(""), userLimit(-1)
{}

Channel::~Channel()
{}

std::string Channel::getName() const
{
    return name;
}

std::string Channel::getTopic() const
{
    return topic;
}

void Channel::setTopic(std::string topic)
{
    this->topic = topic;
}

void Channel::addClient(Client* client)
{
    if (!isClientInChannel(client))
    {
        clients.push_back(client);
    }
}

void Channel::removeClient(Client* client)
{
    for (std::vector<Client*>::iterator it = clients.begin(); it != clients.end(); ++it)
    {
        if (*it == client)
        {
            clients.erase(it);
            break;
        }
    }
    removeOperator(client);
}

void Channel::addOperator(Client* client)
{
    if (!isOperator(client))
    {
        operators.push_back(client);
    }
}

void Channel::removeOperator(Client* client)
{
    for (std::vector<Client*>::iterator it = operators.begin(); it != operators.end(); ++it)
    {
        if (*it == client)
        {
            operators.erase(it);
            break;
        }
    }
}

bool Channel::isClientInChannel(Client* client) const
{
    for (size_t i = 0; i < clients.size(); i++)
    {
        if (clients[i] == client) return true;
    }
    return false;
}

bool Channel::isOperator(Client* client) const
{
    for (size_t i = 0; i < operators.size(); i++)
    {
        if (operators[i] == client) return true;
    }
    return false;
}

void Channel::broadcastMessage(std::string message, Client* excludeClient)
{
    for (size_t i = 0; i < clients.size(); i++)
    {
        if (clients[i] != excludeClient)
        {
            send(clients[i]->getFd(), message.c_str(), message.length(), 0);
        }
    }
}

void Channel::addInvite(std::string nick)
{
    if (!isInvited(nick))
        invitedNicks.push_back(nick);
}

bool Channel::isInvited(std::string nick) const
{
    for (size_t i = 0; i < invitedNicks.size(); i++)
    {
        if (invitedNicks[i] == nick) return true;
    }
    return false;
}

void Channel::removeInvite(std::string nick)
{
    for (std::vector<std::string>::iterator it = invitedNicks.begin(); it != invitedNicks.end(); ++it)
    {
        if (*it == nick)
        {
            invitedNicks.erase(it);
            break;
        }
    }
}

bool Channel::isInviteOnly() const
{
    return inviteOnly;
}

void Channel::setInviteOnly(bool status)
{
    inviteOnly = status;
}

bool Channel::isTopicRestricted() const
{
    return topicRestricted;
}

void Channel::setTopicRestricted(bool status)
{
    topicRestricted = status;
}

std::string Channel::getKey() const
{
    return key;
}

void Channel::setKey(std::string k)
{
    key = k;
}

int Channel::getUserLimit() const
{
    return userLimit;
}

void Channel::setUserLimit(int limit)
{
    userLimit = limit;
}

size_t Channel::getClientCount() const
{
    return clients.size();
}
