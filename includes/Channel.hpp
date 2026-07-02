#ifndef CHANNEL_HPP
#define CHANNEL_HPP

#include <string>
#include <vector>
#include "Client.hpp"

class Channel {
private:
    std::string name;
    std::string topic;
    std::vector<Client*> clients;
    std::vector<Client*> operators;
    std::vector<std::string> invitedNicks;
    
    bool inviteOnly;
    bool topicRestricted;
    std::string key;
    int userLimit; // -1 means no limit

public:
    Channel();
    Channel(std::string name);
    ~Channel();

    std::string getName() const;
    std::string getTopic() const;
    void setTopic(std::string topic);

    void addClient(Client* client);
    void removeClient(Client* client);
    void addOperator(Client* client);
    void removeOperator(Client* client);

    void addInvite(std::string nick);
    bool isInvited(std::string nick) const;
    void removeInvite(std::string nick);

    bool isInviteOnly() const;
    void setInviteOnly(bool status);

    bool isTopicRestricted() const;
    void setTopicRestricted(bool status);

    std::string getKey() const;
    void setKey(std::string key);

    int getUserLimit() const;
    void setUserLimit(int limit);
    
    size_t getClientCount() const;

    bool isClientInChannel(Client* client) const;
    bool isOperator(Client* client) const;

    void broadcastMessage(std::string message, Client* excludeClient);
};

#endif
