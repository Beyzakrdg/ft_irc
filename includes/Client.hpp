
#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <string>
#include <ctime>

class Client {
private:
  int fd;
  std::string displayNick;
  std::string userName;
  std::string hostname;
  bool hasPassword;
  bool successLogin;
  std::string buffer;
  time_t lastPong;   
  time_t lastPingSent; 

public:
  Client(int clientFd);

  int getFd() const;
  std::string getdisplayNick() const;
  std::string getBuffer() const;
  bool getsuccesLogin() const;
  bool getHasPassword() const;
  bool isRegistered() const;
  std::string getuserName() const;
  std::string getHostname() const;
  std::string getPrefix() const;
  time_t getLastPong() const;
  time_t getLastPingSent() const;

  void setdisplayNick(std::string nick);
  void setuserName(std::string user);
  void setHostname(std::string host);
  void setsuccessLogin(bool status);
  void setLastPong(time_t t);
  void setLastPingSent(time_t t);
  void setHasPassword(bool status);

  void appendToBuffer(std::string data);
  void clearBuffer();
  void eraseBuffer(size_t start, size_t length);
};

#endif