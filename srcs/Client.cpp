#include "../includes/Client.hpp"

Client::Client(int clientFd)
    : fd(clientFd), displayNick(""), userName(""), hasPassword(false), successLogin(false),
      buffer("") {}

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
void Client::setdisplayNick(std::string nick)
{
    displayNick = nick;
}
void Client::setuserName(std::string user)
{
    userName = user;
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
