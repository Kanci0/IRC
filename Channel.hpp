#pragma once

#include <string>
#include <vector>
#include <map>
#include <set>
#include "Client.hpp"
#include "Server.hpp"
#include "ModeSplit.hpp"


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
		void	changeMode(std::vector<ModeSplit>, Client);

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
}; 