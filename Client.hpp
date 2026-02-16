#pragma once

#include <string>
#include <vector>

class Client{
	private:
		int fd;
		std::string nickname;
		std::string password;
		std::string username;
		std::string realname;
		bool is_authenticated;
		bool has_user;
		std::vector<char> buffer;
		int len;
		// int operator;
	public:
		Client();
		void set_fd(int fd);
		void set_nick(std::string nick);
		void set_pass(std::string pass);
		void set_username(std::string username);
		bool hasUser() const;
		void set_has_user(bool value);
		void set_authenticated(bool authenticated);
		void set_len(int l);
		const std::vector<char>& getBuffer() const;
		bool get_authenticated();
		int get_len();
		std::string get_nick();
		std::string get_user();
		std::string get_pass();
		void set_realname(const std::string& name);
		std::string get_realname() const;
		int get_fd();
		void appendToBuffer(const char* data, size_t len);
		void removeFromBuffer(size_t len);
};

std::string trimCRLF(std::string& s);