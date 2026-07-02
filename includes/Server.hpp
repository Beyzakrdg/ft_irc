#ifndef SERVER_HPP
#define SERVER_HPP

#include <fcntl.h>
#include <map>
#include <netinet/in.h>
#include <poll.h>
#include <string>
#include <sys/socket.h>
#include <unistd.h>
#include <vector>
#include <iostream>
#include "Client.hpp"
#include "Channel.hpp"

class Server {
private:
  int portNo;
  int serverFd;
  std::string psswd;
  std::vector<struct pollfd> fds;
  std::map<int, std::string> clientBuff;
  std::map<int, Client> clients;
  std::map<std::string, Channel> channels;

  void cmdPass(int sockFd, Client &client, std::vector<std::string> args);
  void cmdNick(int sockFd, Client &client, std::vector<std::string> args);
  void cmdUser(int sockFd, Client &client, std::vector<std::string> args);
  void cmdJoin(int sockFd, Client &client, std::vector<std::string> args);
  void cmdMode(int sockFd, Client &client, std::vector<std::string> args);
  void cmdTopic(int sockFd, Client &client, std::vector<std::string> args);
  void cmdKick(int sockFd, Client &client, std::vector<std::string> args);
  void cmdInvite(int sockFd, Client &client, std::vector<std::string> args);
  void cmdPrivmsg(int sockFd, Client &client, std::vector<std::string> args);

  Client* getClientByNick(std::string nick);
  Channel* getChannelByName(std::string name);
  void sendMessage(int fd, std::string msg);

public:
  Server(int port, std::string password);
  ~Server();

  void init();
  void run();
  bool getClientData(int sockFd);
  void acceptConnection();
  void parseMessage(int sockFd, std::string line);
  void executeCommand(int sockFd, std::string cmd, std::vector<std::string> args);
};

#endif
