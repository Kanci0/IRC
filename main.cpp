#include "Server.hpp"

int main(){
	Server vars;
	struct sockaddr_in my_addr;
	if (vars.SetSocketFd() < 0){
		std::cout << "socket Error";
		return 1; 
	}
	my_addr.sin_family = AF_INET;
	my_addr.sin_port = htons(5150);
	my_addr.sin_addr.s_addr = htonl(INADDR_ANY);

	
	if ((bind(vars.GetSocketFd(), (struct sockaddr *)&my_addr, sizeof(my_addr)) < 0))
	std::cout << "Error";
	if (listen(vars.GetSocketFd(), SOMAXCONN) < 0)
	return perror("listen"), 1;
	
	std::cout << "IRC server listening on port 5100..." << std::endl;
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
		write(1, "hi", 3);
    }
	close(vars.GetSocketFd());
	return (0);
}