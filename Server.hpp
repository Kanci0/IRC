/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: bkaleta <bkaleta@student.42warsaw.pl>      +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/29 21:45:06 by bkaleta           #+#    #+#             */
/*   Updated: 2026/01/29 21:45:36 by bkaleta          ###   ########.fr       */
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
#include <map>
#define MAX_CLIENTS 10

//class Channel;

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
		void SetClientRead();
		void AcceptClient();
		void ClientHandle();
		void Brodcast(const std::string *msg, int len, Client &sender);
		void VerifyCredentials(Client &client);
		int CheckInput(const std::vector<char> buffer, int n, Client &client);
		void JoinHandler(const std::string& buf, Client& client);
		std::vector<std::string> inputHandler();
};

int set_nonblocking(int fd);