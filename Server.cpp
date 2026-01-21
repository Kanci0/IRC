#include "Server.hpp"

Server::Server(){
	num_clients = 0;
}


int set_nonblocking(int fd){
	int flags = fcntl(fd, F_GETFL, 0);
	if (flags == -1)
		return -1;
	return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
};

void Server::Brodcast(const std::string *buf, int len, Client &sender){
	std::string str = "[" + sender.get_nick() + "]: " + *buf;
	(void)len; 
	for (int i = 0; i < num_clients; i++){
		int n = send(Clients[i].get_fd(), str.c_str(), str.size(), 0);
		if (n < 0){
			perror("send error");
			return;
		}
	}
};

int Server::GetMaxFd(){ return max_fd; };
int Server::SetSocketFd(){ return (socket_fd = socket(AF_INET, SOCK_STREAM, 0)); }
void Server::SetMaxFd(int set) { max_fd = set; };
void Server::SetPassword(std::string password){ pass = password; };
int Server::GetSocketFd(){ return socket_fd; };
fd_set& Server::GetReadFds(){ return read_fds; };
std::string Server::GetPass(){ return pass; };
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
			if (new_sock == -1){
				if (errno == EAGAIN || errno == EWOULDBLOCK){
					perror("accept");
					return ;
				}	
			}
			set_nonblocking(new_sock);
			tmp->set_fd(new_sock);
			num_clients++;
			AddClient(*tmp);
			const char *msg = "client connected to chat to connect to channel you need to fill credentials use commands pass - for password/ nick - for nickname\r\n";
			send(new_sock, msg,strlen(msg), 0);
		}
};

void Server::ClientHandle(){
	for (int i = 0; i < num_clients; i++) {
        	if (FD_ISSET(Clients[i].get_fd(), &read_fds) ) {
            	char tmp[4096];
            	int n = recv(Clients[i].get_fd(), tmp, sizeof(tmp), 0);
            	if (n <= 0) {
					close(Clients[i].get_fd());
                	Clients.erase(Clients.begin() + i);
                	num_clients--;
                	i--;
					continue;
				}
				Clients[i].appendToBuffer(tmp, n);
				while (true)
				{
					const std::vector<char>& buf = Clients[i].getBuffer();
					std::vector<char>::const_iterator it = std::find(buf.begin(), buf.end(), '\n');
					if (it == buf.end())
						break;
					int msg_len = it - buf.begin() + 1;
					CheckInput(buf, n, Clients[i]);

					Clients[i].removeFromBuffer(msg_len);
				}
				std::cout << "checking input " << std::endl;
                	// Brodcast(buffer, n, Clients[i].get_fd());
        }
	}
};

void Server::AddClient(Client newClient){
	Clients.push_back(newClient);
}

void Server::VerifyCredentials(Client &client){
	if (client.get_pass() == pass && !client.get_nick().empty()){
		for (int i = 0; i < num_clients; i++){
			if (Clients[i].get_nick() != client.get_nick()){
				continue;
			}
			else if (Clients[i].get_fd() == client.get_fd()){
				continue;
			}
			else{
				const char *msg = "Nickname or password already in use\r\n";
				int n = send(client.get_fd(), msg, strlen(msg), 0);
				if (n < 0){
					perror("send error");
					return;
				}
			}
		}	
		client.set_authenticated(true);
		std::string tmp = "Client " + client.get_nick() + " succesfully aut";
        const char* msg = tmp.c_str();
        int n = send(client.get_fd(), msg, strlen(msg), 0);
		if (n < 0){
			perror("send error");
			return;
		}
	}
};
int Server::CheckInput(const std::vector<char> buffer, int n, Client &client){
	std::string buf(buffer.begin(), buffer.end());
	if(buf.substr(0, 4) == "PASS")
		Commands::PASS(client ,buf.substr(5, std::string::npos));
	else if(buf.substr(0, 4) == "NICK")
		Commands::NICK(client, buf.substr(5, std::string::npos));
	else if(buf.substr(0, 6) == "VERIFY")
		Commands::VERIFY(*this, client);
	else if (client.get_authenticated())
		Brodcast(&buf, n, client);
	else{
		std::string tmp = "Nun authenticated Client " + client.get_nick() + " cannot use chat please fill your password with PASS and nickname with NICK and use VERIFY COMMAND";
        const char* msg = tmp.c_str();
        int n = send(client.get_fd(), msg, strlen(msg), 0);
		if (n < 0){
			perror("send error");
			return -1;
		}
	}
	return 1;
};