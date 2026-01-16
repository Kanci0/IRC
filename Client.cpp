#include "Client.hpp"

void Client::set_fd(int fd){ this->fd = fd; };
int Client::get_fd(){ return fd; };