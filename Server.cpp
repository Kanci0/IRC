#include "Server.hpp"

Server::Server(){
	num_clients = 0;
}

std::vector<std::string> command_split(std::string buf){
	std::vector<std::string> splitted;
	size_t i = 0;
	while(i < buf.size()){
		while(i < buf.size() && buf[i] == ' ')
			i++;
		if (i >= buf.size())
			break;

		size_t space_index = buf.find(' ', i);

		if (space_index == std::string::npos){
			splitted.push_back(buf.substr(i));
			break;
		}
		splitted.push_back(buf.substr(i, space_index - i));

		i = space_index + 1;
	}
	return splitted;
};

int check_flag(char a, int sign){
	if (a == 'k' || a == 'l' || a == 'i' || a == 't' || a == 'o'){
		if ((a == 'k' && sign == 1) || (a == 'l' && sign == 1)
			|| a == 'o')
			return 1;
		else
			return 2;
	}
	return -1;
}

void append_node(std::string str, std::vector<ModeSplit>&parsed, int index){
	ModeSplit tmp;
	int current_sign;
	if (index == 0){
		tmp.value = str;
		tmp.node_number = M_COMMAND;
		tmp.node = "COMMAND";
	}
	if (index == 1){
		tmp.value = str;
		tmp.node_number = M_DESTINATION;
		tmp.node = "DESTINATION";
	}
	if (index == 2){
		for(size_t i = 0; i < str.size(); i++){
			if(str[i] == '-') {
				current_sign = 0;
				tmp.value = str[i];
				tmp.node_number = M_REMOVE;
				tmp.node = "MODE";
			}
			else if(str[i] == '+') {
				current_sign = 1;
				tmp.value = str[i];
				tmp.node_number = M_ADD;
				tmp.node = "MODE";
			}
			else if(check_flag(str[i], current_sign) == 1) {
				tmp.value = str[i];
				tmp.node_number = M_ARGFLAG;
				tmp.node = "FLAG";
			}
			else if(check_flag(str[i], current_sign) == 2) {
				tmp.value = str[i];
				tmp.node_number = M_ARGFLAG;
				tmp.node = "FLAG";
			}
			else {
				tmp.value = str[i];
				tmp.node_number = M_ERROR;
				tmp.node = "ERROR";
			}
		}
	}
	if (index >= 3){
		tmp.value = str;
		tmp.node_number = M_ARG;
		tmp.node = "ARG";
	}
	parsed.push_back(tmp);
}

std::vector<ModeSplit> command_split_moode(std::string buf){
	std::vector<std::string> splitted = command_split(buf);
	std::vector<ModeSplit> parsed;

	for(size_t i = 0; i < splitted.size(); i++){
		append_node(splitted[i], parsed, i);
	}
	return parsed;
};


// enum Command{
// 	CMD_NICK = 1,
// 	CMD_PASS = 2,
// 	CMD_PING = 3,
// 	CMD_KICK = 4,
// 	CMD_INVITE = 5,
// 	CMD_TOPIC = 6,
// 	CMD_MODE = 7
// };
int check_command(std::string buf){
	if(buf.compare(0, 4, "NICK")) return CMD_NICK;
	if(buf.compare(0, 4, "PASS")) return CMD_PASS;
	if(buf.compare(0, 4, "PING")) return CMD_PING;
	if(buf.compare(0, 4, "KICK")) return CMD_KICK;
	if(buf.compare(0, 6, "INVITE")) return CMD_INVITE;
	if(buf.compare(0, 5, "TOPIC")) return CMD_TOPIC;
	if(buf.compare(0, 4, "MODE")) return CMD_MODE;
	return CMD_NONE;
};

int set_nonblocking(int fd){
	int flags = fcntl(fd, F_GETFL, 0);
	if (flags == -1)
		return -1;
	return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
};

void Server::Brodcast(const std::string *buf, int len, Client &sender){
	std::string str = ":" + sender.get_nick() + " " + *buf;
	(void)len; 
	std::cout << "this is string" << std::endl;
	std::cout << str;
	for (int i = 0; i < num_clients; i++){
		if (Clients[i].get_fd() == sender.get_fd())
			continue;
		int n = send(Clients[i].get_fd(), str.c_str(), str.size(), 0);
		if (n < 0){
			perror("send error");
			return;
		}
		std::cout << "message sent" << std::endl;
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

						// WYTNJ JEDNĄ LINIĘ
						std::string line(buf.begin(), buf.begin() + msg_len);
						std::cout << "RAW LINE: [" << line << std::endl;

						// PRZEKAŻ TYLKO JEDNĄ LINIĘ
						CheckInput(
							std::vector<char>(line.begin(), line.end()),
							msg_len,
							Clients[i]
						);

						// USUŃ JĄ Z BUFORA
						Clients[i].removeFromBuffer(msg_len);
				}
				std::cout << "checking input " << std::endl;
                	// Brodcast(buffer, n, Clients[i].get_fd());
        }
	}
};

void Server::AddClient(Client newClient){
	Clients.push_back(newClient);
	// dodanie klienta do kanalu general
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
		std::string tmp = ":server 001 " + client.get_nick() + " :Welcome to the Internet Relay Network\r\n";
        const char* msg = tmp.c_str();
        int n = send(client.get_fd(), msg, strlen(msg), 0);
		if (n < 0){
			perror("send error");
			return;
		}
	}
};

int Server::CheckInput(const std::vector<char> buffer, int n, Client &client)
{
	(void)n;
    std::string buf(buffer.begin(), buffer.end());

	if (buf.compare(0, 4, "PING") == 0)
	{
		std::string token = buf.substr(5);
		std::string pong = "PONG :" + token;
		send(client.get_fd(), pong.c_str(), pong.size(), 0);
		return 1;
	}
    if (buf.substr(0, 4) == "PASS")
        Commands::PASS(client, buf.substr(5));
    else if (buf.substr(0, 4) == "NICK")
        Commands::NICK(client, buf.substr(5));
    else if (buf.compare(0, 4, "USER") == 0)
    {
        // USER bart 0 * :Bartosz\r\n
        size_t pos = buf.find(' ');
        if (pos != std::string::npos)
        {
            size_t start = pos + 1;
            size_t end = buf.find(' ', start);
            if (end != std::string::npos)
            {
                std::string username = buf.substr(start, end - start);
                client.set_username(username);
            }
        }
    }
	else if (buf.compare(0, 4, "JOIN") == 0)
		JoinHandler(buf, client);
	else if (buf.compare(0, 7, "PRIVMSG") == 0){
		Brodcast(&buf, buf.size(), client);
	}

    // 🔑 AUTORYZACJA IRC (PO KAŻDEJ KOMENDZIE)
    if (!client.get_authenticated()
        && !client.get_pass().empty()
        && !client.get_nick().empty()
        && client.hasUser())
    {
		Commands::VERIFY(*this, client);
        if(client.get_authenticated() == true){

        std::string welcome =
            ":ircserv 001 " + client.get_nick() +
            " :Welcome to the Internet Relay Network\r\n";

        send(client.get_fd(), welcome.c_str(), welcome.size(), 0);
		}
    }
    return 1;
}
void Server::JoinHandler(const std::string& buf, Client& client) {
	// rozbic to na funckje
	size_t pos = buf.find(' ');
	if (pos == std::string::npos)
		return ;

	std::string tmp = buf.substr(pos + 1); 
	std::string channel_name = trimCRLF(tmp);
	if (channel_name.empty() || channel_name[0] != '#')
		return ;
	
	if (channels.find(channel_name) != channels.end()){
		channels[channel_name].add_user_to_channel(client);
		std::cout << "client: " << client.get_nick() << " added to existing channel" << std::endl;}
	else
	{
		Channel new_channel;
		new_channel.set_channel_name(channel_name);
		new_channel.add_user_to_channel(client);
		new_channel.add_channel_operator(client);
		channels[channel_name] = new_channel;
		std::cout << "client created channel" << std::endl;
	}
	std::string msg = ":" + client.get_nick() + " JOIN " + channel_name + "\r\n";
	send(client.get_fd(), msg.c_str(), msg.size(), 0);
}