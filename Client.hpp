#pragma once


class Client{
	private:
		int fd;
	public:
		void set_fd(int fd);
		int get_fd();
};