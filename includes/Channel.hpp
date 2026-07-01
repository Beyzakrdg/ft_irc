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

    bool isClientInChannel(Client* client) const;
    bool isOperator(Client* client) const;

    void broadcastMessage(std::string message, Client* excludeClient);
};

#endif
