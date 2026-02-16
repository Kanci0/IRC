#include "Client.hpp"

Client::Client() : fd(-1), nickname(""), password(""), username(""), is_authenticated(false), has_user(false)  {} 


void Client::set_fd(int fd){ this->fd = fd; };
void Client::set_nick(std::string nick){nickname = trimCRLF(nick); };
void Client::set_pass(std::string pass){password = trimCRLF(pass); };
void Client::set_username(std::string user) {username = trimCRLF(user); has_user = true;};
bool Client::hasUser() const { return has_user; };
void Client::set_authenticated(bool authenticated){ is_authenticated = authenticated; };
void Client::set_len(int l){ len = l; };
int Client::get_fd(){ return fd; };
std::string Client::get_nick(){ return nickname; };
std::string Client::get_user(){ return username;};
std::string Client::get_pass(){ return password; };
void Client::set_realname(const std::string& name) { realname = name; };
std::string Client::get_realname() const { return realname; };
bool Client::get_authenticated(){ return is_authenticated; };
void Client::set_has_user(bool value) { has_user = value; };
int Client::get_len() { return len; }
const std::vector<char>& Client::getBuffer() const { return buffer; }
void Client::appendToBuffer(const char* data, size_t len){ buffer.insert(buffer.end(), data, data + len); };
void Client::removeFromBuffer(size_t len){
    if (len > buffer.size()) len = buffer.size();
    buffer.erase(buffer.begin(), buffer.begin() + len);
};

std::string trimCRLF(std::string& s){
    while (!s.empty() && (s[s.size() - 1] == '\n' || s[s.size() - 1] == '\r'))
        s.erase(s.size() - 1);
    return (s);
};

