#include "Channel.hpp"

Channel::Channel() : channel_name(""), topic(""), modes(), key_password(""), users_limit(0), users(), channel_operators() {}

void Channel::set_channel_name(std::string new_name) {
	channel_name = new_name;
}

void Channel::set_topic(std::string new_topic) {
	topic = new_topic;
}

void Channel::add_mode(char new_mode) {
	modes.insert(new_mode);
}

void Channel::remove_mode(char mode_to_remove) {
	modes.erase(mode_to_remove);
}

void Channel::set_keypassword(std::string new_keypassword) {
	key_password = new_keypassword;
}

void Channel::set_users_limit(int new_users_limit) {
	users_limit = new_users_limit;
}

void Channel::add_user_to_channel(Client new_client) {
	users[new_client.get_fd()] = new_client;
}

void Channel::remove_user_from_channel(Client client_to_remove) {
	users.erase(client_to_remove.get_fd());
}

void Channel::add_channel_operator(Client new_channel_operator) {
	channel_operators.insert(new_channel_operator.get_fd());
}

void Channel::remove_channel_operator(Client channel_operator_to_remove) {
	channel_operators.erase(channel_operator_to_remove.get_fd());
}

void	Channel::loadMode(Client client)
{
	std::string mode_str = "+";
	for (std::set<char>::iterator it = modes.begin(); it != modes.end(); ++it)
        mode_str += *it;

	std::string reply =
        ":ircserv 324 " +
        client.get_nick() + " " +
        channel_name + " " +
        mode_str + "\r\n";

    send(client.get_fd(), reply.c_str(), reply.size(), 0);
}
void	Channel::changeMode(std::vector<ModeSplit> res, Client client)
{
	(void)res;
	(void)client;
}

std::string Channel::get_channel_name() {
	return (channel_name);
}

std::string  Channel::get_topic() {
	return (topic);
}

bool Channel::has_mode(char mode) {
	return (modes.find(mode) != modes.end());
}

std::string Channel::get_keypassword() {
	return (key_password);
}

int Channel::get_users_limit() {
	return (users_limit);
}

bool Channel::is_channel_operator(Client channel_operator) {
	return (channel_operators.find(channel_operator.get_fd()) != channel_operators.end());
}

bool Channel::is_channel_user(Client channel_user){
	std::map<int, Client>::iterator it = users.find(channel_user.get_fd());
	if(it != users.end())
		return true;
	return false;
}