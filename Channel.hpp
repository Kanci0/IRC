#pragma once

#include <string>
#include <vector>
#include <map>
#include <set>
#include "Client.hpp"
#include "Server.hpp"
#include "ModeSplit.hpp"
#include <sstream>


class Channel {
	public:
		Channel();
		void	set_channel_name(std::string);
		void	set_topic(std::string);
		void	add_mode(char);
		void	remove_mode(char);
		void 	set_keypassword(std::string);
		void	set_users_limit(int);
		void	add_user_to_channel(Client);
		void	remove_user_from_channel(Client);
		void	add_channel_operator(Client);
		void	remove_channel_operator(Client);
		void	loadMode(Client);
		void	changeMode(const std::vector<ModeSplit> &, Client &);
		void 	invite(int fd);
		bool 	is_invited(int fd) const;
		void 	remove_invite(int fd);
		public:
   		const std::set<char>& get_modes_debug() const;


		std::string get_channel_name();
		std::string get_topic();
		bool		 has_mode(char);
		std::string get_keypassword();
		int				get_users_limit();
		int				get_users_size();
		bool		 is_channel_operator(Client);
		bool		 is_channel_user(Client);
	private:
		std::string 						channel_name;
		std::string							topic;
		std::set<char> 					modes;
		std::string							key_password;
		int 									users_limit;
		std::map<int, Client>	 	users;
		std::set<int> 					channel_operators;
		std::set<int> 					invited_users;
		void broadcastMode(const std::string& mode_change, Client& operator_client);
}; 