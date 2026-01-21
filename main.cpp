#include "Server.hpp"
#include "Commands.hpp"


int main(int argc, char **argv){
	Server vars;
	struct sockaddr_in my_addr;
	if (argc != 3)
		return -1;
	std::string port = argv[1];
	if (!(port.find_first_not_of("0123456789") == std::string::npos)){
		std::cout << "Why am i here mom?" << port;
		return -1;
	}
	if (vars.SetSocketFd() < 0){
		std::cout << "socket Error";
		return 1; 
	}
	std::string pass = argv[2];
	vars.SetPassword(pass);
	my_addr.sin_family = AF_INET;
	my_addr.sin_port = htons(std::atoi(port.c_str()));
	my_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	set_nonblocking(vars.GetSocketFd());

	if ((bind(vars.GetSocketFd(), (struct sockaddr *)&my_addr, sizeof(my_addr)) < 0))
		std::cout << "Error";
	if (listen(vars.GetSocketFd(), SOMAXCONN) < 0)
		return perror("listen"), 1;
	
	std::cout << "IRC server listening on port " << port << std::endl;
	while (1){
		FD_ZERO(&vars.GetReadFds());
		FD_SET(vars.GetSocketFd(), &vars.GetReadFds());
		vars.SetMaxFd(vars.GetSocketFd());
		vars.SetClientRead();
		int activity = select(vars.GetMaxFd() + 1, &vars.GetReadFds(), NULL, NULL, NULL);
		if (activity < 0){
			perror("select");
			continue;
		}
		vars.AcceptClient();
		vars.ClientHandle();
    }
	close(vars.GetSocketFd());
	return (0);
}