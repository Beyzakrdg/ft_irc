#ifndef SERVER_HPP
#define SERVER_HPP

#include <fcntl.h>
#include <iostream>
#include <map>
#include <netinet/in.h>
#include <poll.h>
#include <stdexcept>
#include <string>
#include <sys/socket.h>
#include <unistd.h>
#include <vector>

class Server {
private:
  int portNo;
  int serverFd;
  std::string psswd;
  std::vector<struct pollfd> fds;
  std::map<int, std::string> clientBuff;

public:
  Server(int port, std::string password);
  ~Server();

  void init();
  void run();
  bool getClientData(int sockFd);
  void acceptConnection();
};

#endif
