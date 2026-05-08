#ifndef SERVER_HPP
#define SERVER_HPP

#include <iostream>
#include <vector>
#include <string>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <poll.h>
#include <fcntl.h>
#include <stdexcept>

class Server
{
    private:
        int portNo;
        int serverFd;
        std::string psswd;
        std::vector<struct pollfd> fds;

    public:
        Server(int port, std::string password) : portNo(port), serverFd(-1), psswd(password)
        {
        }
        ~Server()
        {
            if (serverFd != -1)
                close(serverFd);
        }

        void init()
        {
            serverFd = socket(AF_INET, SOCK_STREAM, 0);
            if (serverFd < 0)
                throw std::runtime_error("Socket creation failed");

            int opt = 1;
            setsockopt(serverFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
            fcntl(serverFd, F_SETFL, O_NONBLOCK);

            struct sockaddr_in address;
            address.sin_family = AF_INET;
            address.sin_addr.s_addr = INADDR_ANY;
            address.sin_port = htons(portNo);

            if (bind(serverFd, (struct sockaddr *)&address, sizeof(address)) < 0)
                throw std::runtime_error("Bind failed");

            if (listen(serverFd, 10) < 0)
                throw std::runtime_error("Listen failed");

            struct pollfd serverPollFd;
            serverPollFd.fd = serverFd;
            serverPollFd.events = POLLIN;
            fds.push_back(serverPollFd);
            
            std::cout << "Server started on port " << portNo << std::endl;
        }

        void run()
        {
            while (true)
            {
                if (poll(&fds[0], fds.size(), -1) < 0)
                    throw std::runtime_error("Poll failed");

                for (size_t i = 0; i < fds.size(); i++)
                {
                    if (fds[i].revents & POLLIN)
                    {
                        if (fds[i].fd == serverFd)
                        {
                            acceptConnection();
                        }
                    }
                }
            }
        }
        void acceptConnection()
        {
            struct sockaddr_in clientAddr;
            socklen_t clientLen = sizeof(clientAddr);
            int clientFd = accept(serverFd, (struct sockaddr *)&clientAddr, &clientLen);

            if (clientFd < 0)
            {
                std::cerr << "Accept failed" << std::endl;
                return;
            }

            fcntl(clientFd, F_SETFL, O_NONBLOCK);

            struct pollfd clientPollFd;
            clientPollFd.fd = clientFd;
            clientPollFd.events = POLLIN;
            fds.push_back(clientPollFd);

            std::cout << "New client connected: " << clientFd << std::endl;
        }
};

#endif
