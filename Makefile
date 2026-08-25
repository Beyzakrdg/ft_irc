NAME        = ircserv
CXX         = c++
CXXFLAGS    = -Wall -Wextra -Werror -std=c++98

SRCS_DIR    = srcs/main.cpp srcs/Server.cpp \
              srcs/AuthCommands.cpp srcs/ChannelCommands.cpp srcs/MessageCommands.cpp \
              srcs/Client.cpp srcs/Channel.cpp
INCS_DIR    = includes

SRCS        = $(SRCS_DIR)
OBJS        = $(SRCS:.cpp=.o)

all: $(NAME)

$(NAME): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $(NAME) $(OBJS)

clean:
	rm -f $(OBJS)

fclean: clean
	rm -f $(NAME)

re: fclean all

.PHONY: all clean fclean re