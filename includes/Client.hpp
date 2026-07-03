
#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <string>

class Client {
private:
  int fd;
  std::string displayNick;
  std::string userName;
  bool hasPassword;
  bool successLogin;
  std::string buffer;

public:
  Client(int clientFd);

  int getFd() const;
  std::string getdisplayNick() const;
  std::string getBuffer() const;
  bool getsuccesLogin() const;
  bool getHasPassword() const;
  bool isRegistered() const;
  std::string getuserName() const;

  void setdisplayNick(std::string nick);
  void setuserName(std::string user);
  void setsuccessLogin(bool status);
  void setHasPassword(bool status);

  void appendToBuffer(std::string data);
  void clearBuffer();
  void eraseBuffer(size_t start, size_t length);
};

#endif