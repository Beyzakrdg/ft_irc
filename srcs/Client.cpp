#include "../includes/Client.hpp"

Client::Client(int clientFd)
    : fd(clientFd), displayNick(""), userName(""), hostname("localhost"),
      hasPassword(false), successLogin(false), buffer("") {}

int Client::getFd() const
{
    return fd;
}
std::string Client::getdisplayNick() const
{
    return displayNick;
}
std::string Client::getBuffer() const
{
    return buffer;
}
bool Client::getsuccesLogin() const
{
    return successLogin;
}
bool Client::isRegistered() const
{
    return (hasPassword && !displayNick.empty() && !userName.empty());
}
std::string Client::getuserName() const
{
    return userName;
}
std::string Client::getHostname() const
{
    return hostname;
}
std::string Client::getPrefix() const
{
    std::string user;
    if (userName.empty())
        user = "unknown";
    else
        user = userName;
    std::string host;
    if (hostname.empty())
        host = "localhost";
    else
        host = hostname;
    return displayNick + "!" + user + "@" + host;
}
void Client::setdisplayNick(std::string nick)
{
    displayNick = nick;
}
void Client::setuserName(std::string user)
{
    userName = user;
}
void Client::setHostname(std::string host)
{
    hostname = host;
}
void Client::setsuccessLogin(bool status)
{
    successLogin = status;
}
bool Client::getHasPassword() const
{
    return hasPassword;
}
void Client::setHasPassword(bool status)
{
    hasPassword = status;
}
void Client::appendToBuffer(std::string data)
{
    buffer += data;
}
void Client::clearBuffer()
{
    buffer.clear();
}
void Client::eraseBuffer(size_t start, size_t length)
{
    buffer.erase(start, length);
}
