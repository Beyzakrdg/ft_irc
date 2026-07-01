#include "../includes/Channel.hpp"
#include <sys/socket.h>

Channel::Channel() : name(""), topic("") {}

Channel::Channel(std::string name) : name(name), topic("") {}

Channel::~Channel() {}

std::string Channel::getName() const { return name; }
std::string Channel::getTopic() const { return topic; }
void Channel::setTopic(std::string topic) { this->topic = topic; }

void Channel::addClient(Client* client) {
    if (!isClientInChannel(client)) {
        clients.push_back(client);
    }
}

void Channel::removeClient(Client* client) {
    for (std::vector<Client*>::iterator it = clients.begin(); it != clients.end(); ++it) {
        if (*it == client) {
            clients.erase(it);
            break;
        }
    }
    removeOperator(client);
}

void Channel::addOperator(Client* client) {
    if (!isOperator(client)) {
        operators.push_back(client);
    }
}

void Channel::removeOperator(Client* client) {
    for (std::vector<Client*>::iterator it = operators.begin(); it != operators.end(); ++it) {
        if (*it == client) {
            operators.erase(it);
            break;
        }
    }
}

bool Channel::isClientInChannel(Client* client) const {
    for (size_t i = 0; i < clients.size(); i++) {
        if (clients[i] == client) return true;
    }
    return false;
}

bool Channel::isOperator(Client* client) const {
    for (size_t i = 0; i < operators.size(); i++) {
        if (operators[i] == client) return true;
    }
    return false;
}

void Channel::broadcastMessage(std::string message, Client* excludeClient) {
    for (size_t i = 0; i < clients.size(); i++) {
        if (clients[i] != excludeClient) {
            send(clients[i]->getFd(), message.c_str(), message.length(), 0);
        }
    }
}
