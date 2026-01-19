#include "Client.hpp"

Client::Client(){
    fd = -1;
    nickname = "";
    password = "";
    is_authenticated = false;
}

void Client::set_fd(int fd){ this->fd = fd; };
void Client::set_nick(std::string nick){nickname = nick; };
void Client::set_pass(std::string pass){password = pass; };
void Client::set_authenticated(bool authenticated){ is_authenticated = authenticated; };
int Client::get_fd(){ return fd; };
std::string Client::get_nick(){ return nickname; };
std::string Client::get_pass(){ return password; };
bool Client::get_authenticated(){ return is_authenticated; };