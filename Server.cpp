#include "Server.hpp"

Server::Server(){
	num_clients = 0;
}

void passwd(Client &client, const std::vector<std::string> &pass) {
    if (client.get_authenticated() == true) {
                std::string tmp = "Client " + client.get_nick() + " cannot change password after authentication\r\n";
        const char* msg = tmp.c_str();
        int n = send(client.get_fd(), msg, strlen(msg), 0);
        if (n < 0){
			perror("send error");
			return;
		}
    }
    client.set_pass(pass[1]);

};

void nick(Client &client, const std::vector<std::string> &nick) {
    if (client.get_authenticated() == true ){
        std::string tmp = "Client " + client.get_nick() + " cannot change nickname after authentication\r\n";
        const char* msg = tmp.c_str();
        int n = send(client.get_fd(), msg, strlen(msg), 0);
        if (n < 0){
			perror("send error");
			return;
		}
        return ;
    }
    client.set_nick(nick[1]);
};

void verify(Server &server, Client &client) {
    if (client.get_authenticated()) {
        std::string tmp = "Client " + client.get_nick() + " is already authenticated\r\n";
        int n = send(client.get_fd(), tmp.c_str(), tmp.size(), 0);
        if (n < 0){
			perror("send error");
			return;
		}
        return;
    }
    if (client.get_pass() == server.GetPass() && !client.get_nick().empty()) 
        server.VerifyCredentials(client);
};

void Server::InviteHandler(Client &client, const std::vector<std::string> &invite){
	if (invite.size() != 3)
		return ;
	if (channels.find(invite[2]) != channels.end()){
		if(channels[invite[2]].is_channel_operator(client)){
			Client *target = CheckUserExistance2(invite[1]);
			if (target == NULL)
				return ;
			channels[invite[2]].add_user_to_channel(*target);
			std::string msg = ":localhost 341 " + client.get_nick() + " " + invite[2] + " " + invite[1] + "\r\n";
			send(target->get_fd(), msg.c_str(), msg.size(), 0); 
		}
	}
	
};

std::string assign_topic(std::vector<std::string> s){
	s[2] = s[2].substr(1);
	std::string str;
	for (size_t i = 2; i < s.size(); i++){
		str += s[i];
		if (i + 1 != s.size())
			str += " ";
	}
	return str;
}

void Server::TopicHandler(Client &client, const std::vector<std::string> &topic){
	//Wysyła Topic
	std::string channel_name = topic[1];
	if (topic.size() == 2){
		if (channels.find(channel_name) != channels.end()){
			std::cout << "i went to topic size 2";
			if (channels[channel_name].is_channel_user(client) && !channels[channel_name].get_topic().empty()){
				std::string msg = ":localhost 332 " + client.get_nick() + " " + channel_name + " :" + channels[channel_name].get_topic() + "\r\n"; 
				std::cout << msg;
				send(client.get_fd(), msg.c_str(), msg.size(), 0);
			}
			else if (channels[channel_name].is_channel_user(client)){
				std::string msg = ":localhost 331 " + client.get_nick() + " " + channel_name + " :No topic is set\r\n";
				std::cout << msg;
				send(client.get_fd(), msg.c_str(), msg.size(), 0);
			}
		}
	}
	// Ustawia Topic
	else if (topic.size() >= 3){
		if (channels.find(channel_name) != channels.end()){
			std::cout << "i went to topic size 3 or more";
			if (channels[channel_name].is_channel_operator(client)){
				std::string msg = client.get_nick() + "!" + client.get_user() + "@localhost" + topic[0] + " " + channel_name + " ";
				for (size_t i = 2; i < topic.size(); i++){
					if (i > 2) msg += " ";
					if (i == 2)
					msg += topic[i];
				}
				msg += "\r\n";
				send(client.get_fd(), msg.c_str(), msg.size(), 0);
				channels[channel_name].set_topic(assign_topic(topic));
			}
		}
	}
};

void Server::KickHandler(Client &client, const std::vector<std::string> &kick){
	if (kick.size() <= 2 && kick.size() >= 5)
		return ;
	std::string mssg = "hello\r\n";
	std::cout << mssg;
	send(client.get_fd(), mssg.c_str(), mssg.size(), 0);
	std::string channel_name = kick[1];
	if (channel_name.empty() || channel_name[0] != '#')
		return ;
	
	if (channels.find(channel_name) != channels.end()){
		if (channels[channel_name].is_channel_operator(client)){
			Client *target = CheckUserExistance2(kick[2]);
			if (target == NULL)
				return ;
			if(channels[channel_name].is_channel_user(*target)){
				std::string msg = ":" + client.get_nick() + "!" + client.get_user() + "@localhost" + " " + kick[0] + " " + kick[1] + " " + kick[2] +"\r\n";
				std::cout << msg;
				channels[channel_name].remove_user_from_channel(*target);
				send(target->get_fd(), msg.c_str(), msg.size(), 0);
			}
		}
	}
	

}; 


void print_splitted_mode(std::vector<ModeSplit> res){
	for (size_t i = 0; i < res.size(); i++)
		std::cout << res[i].value << " " << res[i].node << std::endl;
};

std::vector<std::string> command_split(std::string buf){
	std::vector<std::string> splitted;
	size_t i = 0;
	while (i < buf.size()){
		while (i < buf.size() && buf[i] == ' ')
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
		parsed.push_back(tmp);
	}
	if (index == 1){
		tmp.value = str;
		tmp.node_number = M_DESTINATION;
		tmp.node = "DESTINATION";
		parsed.push_back(tmp);
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
			parsed.push_back(tmp);
		}
	}
	if (index >= 3){
		tmp.value = str;
		tmp.node_number = M_ARG;
		tmp.node = "ARG";
		parsed.push_back(tmp);
	}
}

std::vector<ModeSplit> command_split_mode(std::string buf){
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

void Server::Brodcast(const std::string *buf, std::vector<std::string> strr, int len, Client &sender){
	std::string str = ":" + sender.get_nick() + " " + *buf + "\r\n";
	(void)len; 
	std::cout << "this is string" << std::endl;
	std::cout << str;
	std::string channel_name = strr[1];
	if (channels.find(channel_name) != channels.end()){
		if(channels[channel_name].is_channel_user(sender)){
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
Client *Server::CheckUserExistance(Client c){
	for (size_t i = 0; i < Clients.size(); i++){
		if (c.get_fd() == Clients[i].get_fd())
			return &Clients[i];
	}
	return NULL;
}
Client *Server::CheckUserExistance2(std::string s){
	for (size_t i = 0; i < Clients.size(); i++){
		if (!Clients[i].get_nick().compare(s))
			return &Clients[i];
	}
	return NULL;
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
				const char *msg = "Nickname already in use or invalid password\r\n";
				int n = send(client.get_fd(), msg, strlen(msg), 0);
				if (n < 0){
					perror("send error");
					return;
				}
			}
		}	
		client.set_authenticated(true);
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
        passwd(client, command_split(trimCRLF(buf)));
    else if (buf.substr(0, 4) == "NICK")
        nick(client, command_split(trimCRLF(buf)));
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
	else if (buf.compare(0, 6, "INVITE") == 0)
		InviteHandler(client, command_split(trimCRLF(buf)));
	else if (buf.compare(0, 4, "MODE") == 0)
		ModeHandler(command_split_mode(trimCRLF(buf)));
	else if (buf.compare(0, 4, "JOIN") == 0)
		JoinHandler(command_split(trimCRLF(buf)), client);
	else if (buf.compare(0, 7, "PRIVMSG") == 0)
		Brodcast(&buf, command_split(trimCRLF(buf)), buf.size(), client);
	else if (buf.compare(0, 4, "KICK") == 0)
		KickHandler(client, command_split(trimCRLF(buf)));
	else if (buf.compare(0, 5, "TOPIC") == 0)
		TopicHandler(client, command_split(trimCRLF(buf)));

    // 🔑 AUTORYZACJA IRC (PO KAŻDEJ KOMENDZIE)
    if (!client.get_authenticated()
        && !client.get_pass().empty()
        && !client.get_nick().empty()
        && client.hasUser())
    {
		verify(*this, client);
        if(client.get_authenticated() == true){

        std::string welcome =
            ":ircserv 001 " + client.get_nick() +
            " :Welcome to the Internet Relay Network " + client.get_nick() + "\r\n";

        send(client.get_fd(), welcome.c_str(), welcome.size(), 0);
		}
    }
    return 1;
}
void Server::JoinHandler(const std::vector<std::string>& buf, Client& client) {
	// rozbic to na funckje
	if (buf.size() <= 1 || buf.size() >= 4)
		return ;

	std::string channel_name = buf[1];
	if (channel_name.empty() || channel_name[0] != '#')
		return ;
	
	if (channels.find(channel_name) != channels.end()){

		if (!channels[channel_name].get_keypassword().empty() && buf.size() == 3){
			if (buf[2] == channels[channel_name].get_keypassword()){
				channels[channel_name].add_user_to_channel(client);
			}
			else{
				std::cout << "client: " << client.get_nick() << " tried to enter channel with no/wrong password";
				return ;
			}
		}
		else
			channels[channel_name].add_user_to_channel(client);
		std::cout << "client: " << client.get_nick() << " added to existing channel" << std::endl;
	}
	else{
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

void Server::ModeHandler(std::vector<ModeSplit> res){
	if (res.size() < 2)
		return ;
	
	if (res[1].value.empty() || res[1].value[0] != '#')
		return ;
	print_splitted_mode(res);
	for (size_t i = 0; i < res.size(); i++)
		std::cout << "MOJ TEST VALUE\n" << i << ". " << res[i].value << "\n" << std::endl;
	for (size_t i = 0; i < res.size(); i++)
		std::cout << "MOJ TEST NODE\n" << i << ". " << res[i].node << "\n" << std::endl;

	if(res.size() < 3)
		return ;
	// tutej rob mody
}