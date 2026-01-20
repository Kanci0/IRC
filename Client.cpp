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
std::string Client::get_nick(){ return trimCRLF(nickname); };
std::string Client::get_pass(){ return trimCRLF(password); };
bool Client::get_authenticated(){ return is_authenticated; };

std::string trimCRLF(std::string& s){
    while (!s.empty() && (s[s.size() - 1] == '\n' || s[s.size() - 1] == '\r'))
        s.erase(s.size() - 1);
    return (s);
};

