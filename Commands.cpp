#include "Commands.hpp"

void Commands::NICK(Client &client, const std::string &nick) {
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
    client.set_nick(nick);
};

void Commands::PASS(Client &client, const std::string &pass) {
    if (client.get_authenticated() == true) {
                std::string tmp = "Client " + client.get_nick() + " cannot change password after authentication\r\n";
        const char* msg = tmp.c_str();
        int n = send(client.get_fd(), msg, strlen(msg), 0);
        if (n < 0){
			perror("send error");
			return;
		}
    }
    client.set_pass(pass);

};

void Commands::VERIFY(Server &server, Client &client) {
    if (client.get_authenticated()) {
        std::string tmp = "Client " + client.get_nick() + " is already authenticated\r\n";
        int n = send(client.get_fd(), tmp.c_str(), tmp.size(), 0);
        if (n < 0){
			perror("send error");
			return;
		}
        return;
    }
    if (client.get_pass() == server.GetPass() && !client.get_nick().empty()) {
        std::string tmp = "Client " + client.get_nick() + " starting auth\r\n";
        int n = send(client.get_fd(), tmp.c_str(), tmp.size(), 0);
        if (n < 0){
			perror("send error");
			return;
		}
        server.VerifyCredentials(client);
    }
};
