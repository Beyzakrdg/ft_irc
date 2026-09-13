#include "../includes/Client.hpp"

Client::Client(int clientFd)
    : fd(clientFd), displayNick(""), userName(""), hostname("localhost"),
      hasPassword(false), successLogin(false), nickSent(false), buffer("") {}

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
bool Client::getNickSent() const
{
    return nickSent;
}
void Client::setNickSent(bool status)
{
    nickSent = status;
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

static char ircLowerChar(char c)
{
    if (c >= 'A' && c <= 'Z')
        return c + ('a' - 'A');
    if (c == '[')
        return '{';
    if (c == ']')
        return '}';
    if (c == '\\')
        return '|';
    if (c == '~')
        return '^';
    return c;
}

bool Client::nickEquals(const std::string &a, const std::string &b)
{
    if (a.length() != b.length())
        return false;
    for (size_t i = 0; i < a.length(); i++)
    {
        if (ircLowerChar(a[i]) != ircLowerChar(b[i]))
            return false;
    }
    return true;
}

std::string Client::ircLower(const std::string &str)
{
    std::string result = str;
    for (size_t i = 0; i < result.length(); i++)
        result[i] = ircLowerChar(result[i]);
    return result;
}
