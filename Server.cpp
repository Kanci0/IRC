#include "Server.hpp"

Server::Server(){
	num_clients = 0;
}

Server::~Server()
{
    std::cout << "Cleaning up server\n";

    for (size_t i = 0; i < Clients.size(); ++i)
    {
        close(Clients[i].get_fd());
    }

    Clients.clear();
    channels.clear();

    if (socket_fd > 0)
        close(socket_fd);

    std::cout << "Server destroyed properly.\n";
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
			// channels[invite[2]].add_user_to_channel(*target);
			channels[invite[2]].invite(target->get_fd());
			std::string msg = ":localhost 341 " + client.get_nick() + " " + invite[2] + " " + invite[1] + "\r\n";
			send(target->get_fd(), msg.c_str(), msg.size(), 0); 
		}
	}
};

std::string assign_topic(const std::vector<std::string>& s)
{
    if (s.size() < 3)
        return "";

    std::string str;

    // pierwszy fragment
    if (!s[2].empty() && s[2][0] == ':')
        str = s[2].substr(1);
    else
        str = s[2];

    // reszta
    for (size_t i = 3; i < s.size(); ++i)
    {
        str += " ";
        str += s[i];
    }

    return str;
}


void Server::add_topic(Client &client, const std::vector<std::string> &topic){
	std::string channel_name = topic[1];
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

void Server::TopicHandler(Client &client, const std::vector<std::string> &topic)
{
    if (topic.size() < 2)
        return;

    const std::string& channel_name = topic[1];

    std::map<std::string, Channel>::iterator it = channels.find(channel_name);
    if (it == channels.end())
        return;

    Channel &channel = it->second;

    // ===== GET TOPIC =====
    if (topic.size() == 2)
    {
        if (!channel.is_channel_user(client))
            return;

        if (!channel.get_topic().empty())
        {
            std::string msg = ":ircserv 332 " + client.get_nick() +
                              " " + channel_name + " :" +
                              channel.get_topic() + "\r\n";
            send(client.get_fd(), msg.c_str(), msg.size(), 0);
        }
        else
        {
            std::string msg = ":ircserv 331 " + client.get_nick() +
                              " " + channel_name + " :No topic is set\r\n";
            send(client.get_fd(), msg.c_str(), msg.size(), 0);
        }
        return;
    }

    // ===== SET TOPIC =====
    if (!channel.is_channel_user(client))
        return;

    // +t → tylko operator
    if (channel.has_mode('t') && !channel.is_channel_operator(client))
    {
        std::string msg = ":ircserv 482 " + client.get_nick() +
                          " " + channel_name +
                          " :You're not channel operator\r\n";
        send(client.get_fd(), msg.c_str(), msg.size(), 0);
        return;
    }

    add_topic(client, topic);
}

void Server::KickHandler(Client &client, const std::vector<std::string> &kick)
{
    if (kick.size() < 3)
        return;

    std::string channel_name = kick[1];

    if (channel_name.empty() || channel_name[0] != '#')
        return;

    if (channels.find(channel_name) == channels.end())
        return;

    Channel &channel = channels[channel_name];

    if (!channel.is_channel_operator(client))
        return;

    Client *target = CheckUserExistance2(kick[2]);
    if (!target)
        return;

    if (!channel.is_channel_user(*target))
        return;

    std::string msg = ":" + client.get_nick() + "!" +
                      client.get_user() + "@localhost KICK " +
                      channel_name + " " + kick[2] + "\r\n";

    // 🔥 broadcast do wszystkich userów kanału
    std::map<int, Client>& users = channel.get_users_map();
    for (std::map<int, Client>::iterator it = users.begin();
         it != users.end(); ++it)
    {
        send(it->first, msg.c_str(), msg.size(), 0);
    }

    // usuń usera z kanału
    channel.remove_user_from_channel(*target);
}

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

void Server::privmsg_server(std::string msg, std::string target, Client &sender){
	if (channels.find(target) != channels.end()){
		if(channels[target].is_channel_user(sender)){
			for (int i = 0; i < num_clients; i++){
				if (Clients[i].get_fd() == sender.get_fd())
					continue;
				if (channels[target].is_channel_user(Clients[i])){
					int n = send(Clients[i].get_fd(), msg.c_str(), msg.size(), 0);
					if (n < 0){
						perror("send error");
						return;
					}
					std::cout << "message sent" << std::endl;
				}
			}
		}
	}
}

void Server::privmsg(std::string msg, std::string target){
	Client *reciver = CheckUserExistance2(target);
	if (reciver != NULL){
		int n = send(reciver->get_fd(), msg.c_str(), msg.size(), 0);
		if (n < 0){
			perror("send error");
			return;
		}
	}
}

void Server::Brodcast(const std::string *buf, std::vector<std::string> strr, int len, Client &sender){
	std::string str = ":" + sender.get_nick() + " " + *buf + "\r\n";
	(void)len; 
	std::cout << "this is string" << std::endl;
	std::cout << str;
	std::string target = strr[1];
	if (target[0] == '#')
		privmsg_server(str, target, sender);
	else 
		privmsg(str, target);
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
			Client tmp;
			int new_sock = accept(socket_fd, NULL, NULL);
			if (new_sock == -1){
				if (errno == EAGAIN || errno == EWOULDBLOCK){
					perror("accept");
					return ;
				}	
			}
			set_nonblocking(new_sock);
			tmp.set_fd(new_sock);
			num_clients++;
			AddClient(tmp);
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
		std::vector<std::string> tokens = command_split(trimCRLF(buf));

		if (tokens.size() < 5)
			return 0;

		client.set_username(tokens[1]);

		// Jeśli jest dwukropek → wszystko po nim to realname
		size_t colon_pos = buf.find(':');
		if (colon_pos != std::string::npos)
		{
			std::string realname = buf.substr(colon_pos + 1);
			client.set_realname(realname);
		}
		else
		{
			// brak ':' → realname to 4-ty token
			client.set_realname(tokens[4]);
		}

		client.set_has_user(true);

		std::cout << "USER parsed: "
				<< client.get_user()
				<< " | "
				<< client.get_realname()
				<< std::endl;
	}
	else if (buf.compare(0, 6, "INVITE") == 0)
		InviteHandler(client, command_split(trimCRLF(buf)));
	else if (buf.compare(0, 4, "MODE") == 0)
		ModeHandler(command_split_mode(trimCRLF(buf)), client);
	else if (buf.compare(0, 4, "JOIN") == 0)
		JoinHandler(command_split(trimCRLF(buf)), client);
	else if (buf.compare(0, 7, "PRIVMSG") == 0)
		Brodcast(&buf, command_split(trimCRLF(buf)), buf.size(), client);
	else if (buf.compare(0, 4, "KICK") == 0)
		KickHandler(client, command_split(trimCRLF(buf)));
	else if (buf.compare(0, 5, "TOPIC") == 0)
		TopicHandler(client, command_split(trimCRLF(buf)));
	else if (buf.compare(0, 4, "LIST") == 0)
    	ListHandler(client, command_split(trimCRLF(buf)));
	else if (buf.compare(0, 3, "WHO") == 0)
    	WhoHandler(client, command_split(trimCRLF(buf)));

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

bool Server::join(const std::vector<std::string> buf, Client& client)
{
    std::string channel_name = buf[1];
    Channel &channel = channels[channel_name];

    // ===== +l (limit users) =====
    if (channel.has_mode('l') && 
        channel.get_users_size() >= channel.get_users_limit())
    {
        std::string msg = ":localhost 471 " + client.get_nick() +
                          " " + channel_name +
                          " :Cannot join channel (+l)\r\n";
        send(client.get_fd(), msg.c_str(), msg.size(), 0);
        return false;
    }

    // ===== +k (channel key) =====
    std::string key = channel.get_keypassword();

    if (!key.empty())
    {
        // brak hasła
        if (buf.size() < 3)
        {
            std::string msg = ":localhost 475 " + client.get_nick() +
                              " " + channel_name +
                              " :Cannot join channel (+k)\r\n";
            send(client.get_fd(), msg.c_str(), msg.size(), 0);
            return false;
        }

        // złe hasło
        if (buf[2] != key)
        {
            std::string msg = ":localhost 475 " + client.get_nick() +
                              " " + channel_name +
                              " :Cannot join channel (+k)\r\n";
            send(client.get_fd(), msg.c_str(), msg.size(), 0);
            return false;
        }
    }

    // ===== OK → dodaj do kanału =====
    channel.add_user_to_channel(client);

    std::cout << "client: " << client.get_nick()
              << " added to existing channel" << std::endl;
	return true;
}

void Server::JoinHandler(const std::vector<std::string>& buf, Client& client)
{
    if (buf.size() <= 1 || buf.size() >= 4)
        return;

    std::string channel_name = buf[1];
    if (channel_name.empty() || channel_name[0] != '#')
        return;

    // 1️⃣ kanał NIE istnieje → tworzymy
    if (channels.find(channel_name) == channels.end())
    {
        Channel new_channel;
        new_channel.set_channel_name(channel_name);
        new_channel.add_user_to_channel(client);
        new_channel.add_channel_operator(client);
        channels[channel_name] = new_channel;

        std::string msg = ":" + client.get_nick() + " JOIN " + channel_name + "\r\n";
        send(client.get_fd(), msg.c_str(), msg.size(), 0);
		NamesHandler(client, channel_name);
        return;
    }

    // 2️⃣ kanał JUŻ istnieje → pracujemy na REFERENCJI
    Channel &channel = channels[channel_name];

    // 3️⃣ +i (invite-only)
    if (channel.has_mode('i'))
    {
        if (!channel.is_invited(client.get_fd()))
        {
            std::string msg = ":localhost 473 " + client.get_nick() +
                              " " + channel_name + " :Cannot join channel (+i)\r\n";
            send(client.get_fd(), msg.c_str(), msg.size(), 0);
            return;
        }
        channel.remove_invite(client.get_fd());
    }

	if (join(buf, client))
	{
		Channel &channel = channels[channel_name];

		std::string msg = ":" + client.get_nick() + " JOIN " + channel_name + "\r\n";

		std::map<int, Client>::iterator it = channel.get_users_map().begin();
		for (; it != channel.get_users_map().end(); ++it)
		{
			send(it->first, msg.c_str(), msg.size(), 0);
		}
		NamesHandler(client, channel_name);
	}

}

void Server::ModeHandler(const std::vector<ModeSplit> &res, Client& client){
	if (res.size() < 2)
		return ;
	
	if (res[1].value.empty() || res[1].value[0] != '#')
		return ;

	const std::string& channel_name = res[1].value;
	std::map<std::string, Channel>::iterator it = channels.find(channel_name);
    if (it == channels.end())
        return;

    Channel& channel = it->second;

	if (res.size() == 2)
		channel.loadMode(client);
	else if (res.size() >= 3)
		channel.changeMode(res, client);
}

void Channel::broadcastMode(const std::string& mode_change, Client& operator_client)
{
    std::string message =
        ":" + operator_client.get_nick() +
        "!" + operator_client.get_user() +
        "@localhost MODE " +
        channel_name + " " +
        mode_change + "\r\n";

    for (std::map<int, Client>::iterator it = users.begin();
         it != users.end();
         ++it)
    {
        send(it->first, message.c_str(), message.size(), 0);
    }
}

void Server::ListHandler(Client& client, const std::vector<std::string>& cmd)
{
    (void)cmd;

    // 321 - start listy
    std::string start =
        ":ircserv 321 " +
        client.get_nick() +
        " Channel :Users Name\r\n";

    send(client.get_fd(), start.c_str(), start.size(), 0);

    // iteracja po kanałach
    std::map<std::string, Channel>::iterator it = channels.begin();
    for (; it != channels.end(); ++it)
    {
        Channel& channel = it->second;

        std::stringstream ss;
        ss << channel.get_users_size();

        std::string topic = channel.get_topic();
        if (topic.empty())
            topic = "";

        std::string entry =
            ":ircserv 322 " +
            client.get_nick() + " " +
            channel.get_channel_name() + " " +
            ss.str() + " :" +
            topic + "\r\n";

        send(client.get_fd(), entry.c_str(), entry.size(), 0);
    }

    // 323 - end list
    std::string end =
        ":ircserv 323 " +
        client.get_nick() +
        " :End of /LIST\r\n";

    send(client.get_fd(), end.c_str(), end.size(), 0);
}

void Server::WhoHandler(Client& client, const std::vector<std::string>& cmd)
{
    if (cmd.size() < 2)
        return;

    std::string channel_name = cmd[1];

    if (channels.find(channel_name) == channels.end())
        return;

    Channel& channel = channels[channel_name];

    std::map<int, Client>& users = channel.get_users_map(); // potrzebujesz getter

    for (std::map<int, Client>::iterator it = users.begin(); it != users.end(); ++it)
    {
        Client& target = it->second;

        std::string reply =
            ":ircserv 352 " +
            client.get_nick() + " " +
            channel_name + " " +
            target.get_user() + " localhost ircserv " +
            target.get_nick() +
            " H :0 " +
            target.get_realname() +
            "\r\n";

        send(client.get_fd(), reply.c_str(), reply.size(), 0);
    }

    std::string end =
        ":ircserv 315 " +
        client.get_nick() + " " +
        channel_name +
        " :End of /WHO list\r\n";

    send(client.get_fd(), end.c_str(), end.size(), 0);
}

void Server::NamesHandler(Client& client, const std::string& channel_name)
{
    if (channels.find(channel_name) == channels.end())
        return;

    Channel& channel = channels[channel_name];

    std::string names_list = "";

    std::map<int, Client>& users = channel.get_users_map();

    for (std::map<int, Client>::iterator it = users.begin(); it != users.end(); ++it)
    {
        Client& target = it->second;

        if (channel.is_channel_operator(target))
            names_list += "@";

        names_list += target.get_nick();
        names_list += " ";
    }

    std::string reply =
        ":ircserv 353 " +
        client.get_nick() +
        " = " +
        channel_name +
        " :" +
        names_list +
        "\r\n";

    send(client.get_fd(), reply.c_str(), reply.size(), 0);

    std::string end =
        ":ircserv 366 " +
        client.get_nick() +
        " " +
        channel_name +
        " :End of /NAMES list\r\n";

    send(client.get_fd(), end.c_str(), end.size(), 0);
}

