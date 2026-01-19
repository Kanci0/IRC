#pragma once

#include <string>

class Client{
	private:
		int fd;
		std::string nickname;
		std::string password;
		bool is_authenticated;
	public:
		Client();
		void set_fd(int fd);
		void set_nick(std::string nick);
		void set_pass(std::string pass);
		void set_authenticated(bool authenticated);
		bool get_authenticated();
		std::string get_nick();
		std::string get_pass();
		int get_fd();
};