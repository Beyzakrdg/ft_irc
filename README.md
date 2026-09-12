*This project has been created as part of the 42 curriculum by elikavak, bekarada, hugozlu.*

# ft_irc — Internet Relay Chat Server

A fully functional IRC server written in C++98, built as part of the 42 curriculum. It supports multiple simultaneous clients via non-blocking I/O multiplexing (`poll()`), and implements core IRC protocol commands compatible with standard IRC clients such as **irssi**.

---

## Description

`ft_irc` is a custom IRC server that allows multiple clients to connect simultaneously and communicate through channels and private messages. The server runs on a single thread using non-blocking sockets and `poll()` for I/O multiplexing — no `fork()`, no threads.

All messages sent by the server use the standard IRC prefix format `nick!user@host`, ensuring full compatibility with clients like **irssi**.

### Supported Commands

| Command   | Description                                         |
|-----------|-----------------------------------------------------|
| `PASS`    | Authenticate with the server password               |
| `NICK`    | Set or change nickname                              |
| `USER`    | Set username and real name (completes registration) |
| `JOIN`    | Join a channel (created if it doesn't exist)        |
| `PART`    | Leave a channel                                     |
| `PRIVMSG` | Send a message to a user or channel                 |
| `NOTICE`  | Send a notice to a user or channel                  |
| `KICK`    | Remove a user from a channel (operator only)        |
| `INVITE`  | Invite a user to a channel (operator only)          |
| `TOPIC`   | View or change a channel's topic                    |
| `MODE`    | Set channel modes (see below)                       |
| `PING`    | Client-initiated connection check; server replies with `PONG` |
| `PONG`    | Response to server-initiated keepalive `PING`       |
| `QUIT`    | Disconnect from the server                          |

### Channel Modes (via `MODE`)

| Mode | Description                     |
|------|---------------------------------|
| `+i` | Invite-only channel             |
| `+t` | Topic restricted to operators   |
| `+k` | Channel key (password)          |
| `+o` | Grant/revoke operator privilege |
| `+l` | Set user limit                  |

---

## Keepalive — Server-Side Ping/Pong

The server automatically monitors all connected clients using a periodic ping mechanism:

- Every **90 seconds** of inactivity, the server sends `PING :server` to the client.
- The client must reply with `PONG :server` within **120 seconds**.
- If no `PONG` is received within the timeout window, the client is disconnected with the reason `Ping timeout` and all joined channels are notified.

These intervals are defined as compile-time constants in `includes/Server.hpp`:

```cpp
#define PING_INTERVAL 90   // seconds between pings
#define PING_TIMEOUT  120  // seconds to wait for PONG before disconnecting
```

---

## Instructions

### Requirements

- A C++ compiler supporting C++98 (e.g. `clang++` or `g++`)
- macOS or Linux

### Build

```bash
make
```

This produces the `ircserv` binary in the project root.

### Run

```bash
./ircserv <port> <password>
```

- `<port>` — A valid TCP port number (1–65535). Recommended: use a port above 1024 (e.g. `6667`).
- `<password>` — The connection password that IRC clients must provide.

**Example:**
```bash
./ircserv 6667 mypassword
```

### Connect with irssi

```bash
irssi -c localhost -p 6667 -w mypassword -n yournick
```

### Connect with netcat (manual testing)

```bash
nc -c localhost 6667
PASS mypassword
NICK mynick
USER myuser 0 * :My Real Name
JOIN #lobby
PRIVMSG #lobby :Hello World!
```

### Clean build

```bash
make re       # full rebuild
make clean    # remove object files
make fclean   # remove object files and binary
```

### Stop the server

Press `Ctrl+C` (SIGINT) to gracefully shut down the server. All connected clients will receive a shutdown notice and all sockets will be cleanly closed.

---

## Resources

The following resources were used to understand the IRC protocol and socket programming in C++:

- [RFC 1459 — Internet Relay Chat Protocol](https://datatracker.ietf.org/doc/html/rfc1459)
- [RFC 2812 — IRC Client Protocol](https://datatracker.ietf.org/doc/html/rfc2812)
- [Beej's Guide to Network Programming](https://beej.us/guide/bgnet/)
- [Modern IRC Client Protocol (ircv3)](https://ircv3.net/)
- [man 2 poll](https://man7.org/linux/man-pages/man2/poll.2.html)
- [man 7 socket](https://man7.org/linux/man-pages/man7/socket.7.html)

AI tools were occasionally consulted for research and documentation purposes. All code was written and understood by the project authors.
