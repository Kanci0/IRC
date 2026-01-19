#pragma once

#include "Client.hpp"
#include "Server.hpp"

class Server;

class Commands
{
public:
    static void NICK(Client &client, const std::string &nick);
    static void PASS(Client &client, const std::string &pass);
    static void VERIFY(Server &server, Client &client);
};

