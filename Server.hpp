/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: bkaleta <bkaleta@student.42warsaw.pl>      +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/29 21:45:06 by bkaleta           #+#    #+#             */
/*   Updated: 2026/02/05 22:45:31 by bkaleta          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once

#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <iostream>
#include <strings.h>
#include <cstring>
#include <unistd.h>
#include <cstdio>
#include <vector>
#include <fcntl.h>
#include <algorithm>
#include <cerrno>
#include "Client.hpp"
#include "Commands.hpp"
#include "Channel.hpp"
#include "ModeSplit.hpp"
#include <map>
#define MAX_CLIENTS 10

//class Channel;

enum Command{
	CMD_NICK = 1,
	CMD_PASS = 2,
	CMD_PING = 3,
	CMD_KICK = 4,
	CMD_INVITE = 5,
	CMD_TOPIC = 6,
	CMD_MODE = 7,
	CMD_NONE = 8
};

enum ModeNode{
	M_COMMAND = 1,
	M_DESTINATION = 2,
	M_REMOVE = 3,
	M_ADD = 4,
	M_FLAG = 5,
	M_ARGFLAG = 6,
	M_ARG = 7,
	M_ERROR = 666
};

// struct ModeSplit{
// 	std::string value;
// 	std::string node;
// 	int node_number;
// };

class Server{
	private:
		int socket_fd;
		int max_fd;
		int num_clients;
		std::vector<Client> Clients;
		std::map<std::string, Channel> channels;
		std::string pass;
		fd_set read_fds;
	public:
		Server();
		int SetSocketFd();
		void SetMaxFd(int set);
		void SetNumClients(int set);
		void SetPassword(std::string password);
		fd_set &GetReadFds();
		int GetSocketFd();
		int GetMaxFd();
		int GetNumClients();
		std::string GetPass();
		void DecrementNumClients();
		void AddClient(Client newClient);
		Client *CheckUserExistance(Client c);
		Client *CheckUserExistance2(std::string s);
		void SetClientRead();
		void AcceptClient();
		void ClientHandle();
		void Brodcast(const std::string *msg, std::vector<std::string> strr, int len, Client &sender);
		void VerifyCredentials(Client &client);
		int CheckInput(const std::vector<char> buffer, int n, Client &client);
		void JoinHandler(const std::vector<std::string>& buf, Client& client);
		void ModeHandler(const std::vector<ModeSplit> &res, Client& client);
		void KickHandler(Client &client, const std::vector<std::string> &kick); 
		void InviteHandler(Client &client, const std::vector<std::string> &invite);
		void TopicHandler(Client &client, const std::vector<std::string> &topic);
		std::vector<std::string> inputHandler();
};

int set_nonblocking(int fd);
std::vector<std::string> command_split(std::string buf, int option);
std::vector<ModeSplit> command_split_mode(std::string buf);
int check_command(std::string buf);
void print_splitted_mode(std::vector<ModeSplit> res);
void passwd(Client &client, const std::vector<std::string> &pass);
void nick(Client &client, const std::vector<std::string> &nick);
