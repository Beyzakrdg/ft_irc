#ifndef SERVER_HPP
#define SERVER_HPP

#include <arpa/inet.h>
#include <fcntl.h>
#include <map>
#include <netinet/in.h>
#include <poll.h>
#include <signal.h>
#include <string>
#include <sys/socket.h>
#include <unistd.h>
#include <vector>
#include <iostream>
#include <ctime>
#include <cctype>
#include <cstdlib>
#include <cstdio>
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
  std::map<int, std::string> outBuffers;
  std::map<int, Client> clients;
  std::map<std::string, Channel> channels;

  static volatile sig_atomic_t _running;
  static void signalHandler(int signum);
  void shutdown();
  void cmdPass(int sockFd, Client &client, std::vector<std::string> args);
  void cmdNick(int sockFd, Client &client, std::vector<std::string> args);
  void cmdUser(int sockFd, Client &client, std::vector<std::string> args);
  void cmdJoin(int sockFd, Client &client, std::vector<std::string> args);
  void cmdMode(int sockFd, Client &client, std::vector<std::string> args);
  void cmdTopic(int sockFd, Client &client, std::vector<std::string> args);
  void cmdKick(int sockFd, Client &client, std::vector<std::string> args);
  void cmdInvite(int sockFd, Client &client, std::vector<std::string> args);
  void cmdPrivmsg(int sockFd, Client &client, std::vector<std::string> args);
  void cmdNotice(int sockFd, Client &client, std::vector<std::string> args);
  void cmdPing(int sockFd, Client &client, std::vector<std::string> args);
  void cmdPong(int sockFd, Client &client, std::vector<std::string> args);
  void cmdQuit(int sockFd, Client &client, std::vector<std::string> args);
  void cmdPart(int sockFd, Client &client, std::vector<std::string> args);

  void disconnectClient(int sockFd);
  void flushOutBuffer(int sockFd);
  void updatePollEvents(int fd, short events);


  Client*  getClientByNick(std::string nick);
  Channel* getChannelByName(std::string name);
  void     sendMessage(int fd, std::string msg);


  Channel* getValidChannel(int sockFd, Client &client, const std::string &name);
  bool     checkInChannel(int sockFd, Client &client, Channel &chan, const std::string &name);
  bool     checkIsOperator(int sockFd, Client &client, Channel &chan, const std::string &name);


  void handleModeI(Client &client, Channel &chan, const std::string &target, bool adding);
  void handleModeT(Client &client, Channel &chan, const std::string &target, bool adding);
  void handleModeK(Client &client, Channel &chan, const std::string &target, bool adding,
                   std::vector<std::string> &args, size_t &argIndex);
  void handleModeO(int sockFd, Client &client, Channel &chan, const std::string &target, bool adding,
                   std::vector<std::string> &args, size_t &argIndex);
  void handleModeL(Client &client, Channel &chan, const std::string &target, bool adding,
                   std::vector<std::string> &args, size_t &argIndex);

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
