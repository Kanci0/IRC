#include "Server.hpp"


Server::Server(){
	num_clients = 0;
}

void Server::Brodcast(const char *msg, int len, int sender_sock){
	for (int i = 0; i < num_clients; i++){
		if (Clients[i].get_fd() != sender_sock){
			int n = send(Clients[i].get_fd(),msg, len, 0);
			if (n < 0){
				perror("send error");
				return;
			}
		}
	}
};

int Server::GetMaxFd(){ return max_fd; };
int Server::SetSocketFd(){ return (socket_fd = socket(AF_INET, SOCK_STREAM, 0)); }
void Server::SetMaxFd(int set) { max_fd = set; };
int Server::GetSocketFd(){ return socket_fd; };
fd_set& Server::GetReadFds(){ return read_fds; };
void Server::DecrementNumClients(){ num_clients--; };
void Server::SetClientRead(){
	for (int i = 0; i < num_clients; i++){
		FD_SET(Clients[i].get_fd(), &read_fds);
		if (Clients[i].get_fd() > max_fd)
			max_fd = Clients[i].get_fd();
	}
};

void Server::AcceptClient(){
		if (FD_ISSET(socket_fd, &read_fds)){
			Client *tmp = new Client();
			int new_sock = accept(socket_fd, NULL, NULL);
			tmp->set_fd(new_sock);
			AddClient(*tmp);
			const char *msg = "client connected to chat to connect to channel you need to fill credentials use commands pass - for password/ nick - for nickname\r\n";
			send(new_sock, msg,strlen(msg), 0);
		}
};

void Server::ClientHandle(){
	for (int i = 0; i < num_clients; i++) {
        	if (FD_ISSET(Clients[i].get_fd(), &read_fds)) {
            	char buffer[1024];
            	int n = recv(Clients[i].get_fd(), buffer, sizeof(buffer), 0);
            	if (n <= 0) {
                	close(Clients[i].get_fd());
                	Clients.erase(Clients.begin() + i);
                	num_clients--;
                	i--;
            	} else {
                	Brodcast(buffer, n, Clients[i].get_fd());
            	}
        }
	}
};

void Server::AddClient(Client newClient){
	Clients.push_back(newClient);
}