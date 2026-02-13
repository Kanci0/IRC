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

void Channel::loadMode(Client client)
{
    std::string mode_str = "+";
    std::string params = "";

    for (std::set<char>::iterator it = modes.begin(); it != modes.end(); ++it)
    {
        mode_str += *it;
        if (*it == 'l')
        {
            params += " ";
            std::stringstream ss;
			ss << users_limit;
			params += ss.str();
        }
    }

    std::string reply =
        ":ircserv 324 " +
        client.get_nick() + " " +
        channel_name + " " +
        mode_str +
        params +
        "\r\n";

    send(client.get_fd(), reply.c_str(), reply.size(), 0);
}

void	Channel::changeMode(const std::vector<ModeSplit> &res, Client &client)
{
	if (!is_channel_operator(client))
		return ;

	if (res.size() < 4)
		return ;

	if (res[2].value == "+")
        handleAddMode(res);
    else if (res[2].value == "-")
        handleRemoveMode(res);
}

void	Channel::handleAddMode(const std::vector<ModeSplit>& res)
{
	const std::string &flag = res[3].value;
	if (flag == "i") {
		modes.insert('i');
		return;
	}
	else if (flag == "t") {
		modes.insert('t');
		return;
	}
	else if (flag == "k") {
		if (res.size() < 5)
			return ;
		
		const std::string& password = res[4].value;
		if (password.empty())
			return;

		key_password = password;
		modes.insert('k');
		return;
	}
	else if (flag == "l")
	{
		if (res.size() < 5)
			return;

		const std::string& limit_str = res[4].value;

		for (size_t i = 0; i < limit_str.size(); ++i)
			if (!isdigit(limit_str[i]))
				return;

		int limit = atoi(limit_str.c_str());
		if (limit <= 0)
			return;

		users_limit = limit;
		modes.insert('l');
		return;
	}
	else if (flag ==  "o")
	{
		if (res.size() < 5)
			return;

		const std::string& user_to_adjust = res[4].value;
		std::map<int, Client>::iterator it = users.begin();
		for (; it != users.end(); ++it)
		{
			if (it->second.get_nick() == user_to_adjust)
			{
				channel_operators.insert(it->first);
				return;
			}
		}
	}	
}

void Channel::handleRemoveMode(const std::vector<ModeSplit>& res)
{
    if (res.size() < 4)
        return;

    const std::string& flag = res[3].value;

    if (flag == "i")
    {
        modes.erase('i');
        return;
    }
    else if (flag == "t")
    {
        modes.erase('t');
        return;
    }
    else if (flag == "k")
    {
        key_password.clear();
        modes.erase('k');
        return;
    }
    else if (flag == "l")
    {
        users_limit = 0;
        modes.erase('l');
        return;
    }
    else if (flag == "o")
    {
        if (res.size() < 5)
            return;

        const std::string& user_to_adjust = res[4].value;

        std::map<int, Client>::iterator it = users.begin();
        for (; it != users.end(); ++it)
        {
            if (it->second.get_nick() == user_to_adjust)
            {
                channel_operators.erase(it->first);
                return;
            }
        }
    }
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

int	Channel::get_users_size(){
	return (users.size());
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

void Channel::invite(int fd)
{
    invited_users.insert(fd);
}

bool Channel::is_invited(int fd) const
{
    return invited_users.find(fd) != invited_users.end();
}

void Channel::remove_invite(int fd)
{
    invited_users.erase(fd);
}

const std::set<char>& Channel::get_modes_debug() const
{
	return (modes);
}