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
    int fd = client_to_remove.get_fd();

    users.erase(fd);
    channel_operators.erase(fd); 
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

    if (has_mode('i'))
        mode_str += "i";

    if (has_mode('t'))
        mode_str += "t";

    if (has_mode('k'))
        mode_str += "k";

    if (has_mode('l'))
    {
        mode_str += "l";

        std::stringstream ss;
        ss << users_limit;
        params += " ";
        params += ss.str();
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

void Channel::changeMode(const std::vector<ModeSplit>& res, Client& client)
{
    if (!is_channel_operator(client))
        return;

    bool adding = true;

    size_t param_index = 2;

    while (param_index < res.size())
    {
        std::string v = res[param_index].value;

        if (v == "+" || v == "-")
        {
            param_index++;
            continue;
        }

        if (v.size() == 1 &&
            (v[0] == 'i' || v[0] == 't' || v[0] == 'k' ||
            v[0] == 'l' || v[0] == 'o'))
        {
            param_index++;
            continue;
        }

        break;
    }


    for (size_t i = 2; i < res.size(); ++i)
    {
        const std::string& token = res[i].value;

        if (token == "+")
        {
            adding = true;
            continue;
        }

        if (token == "-")
        {
            adding = false;
            continue;
        }

        if (token.size() != 1)
            continue;

        char flag = token[0];

        if (flag == 'i')
        {
            if (adding)
            {
                modes.insert('i');
                broadcastMode("+i", client);
            }
            else
            {
                modes.erase('i');
                broadcastMode("-i", client);
            }
        }

        else if (flag == 't')
        {
            if (adding)
            {
                modes.insert('t');
                broadcastMode("+t", client);
            }
            else
            {
                modes.erase('t');
                broadcastMode("-t", client);
            }
        }

        else if (flag == 'k')
        {
            if (adding)
            {
                if (param_index >= res.size())
                    return;

                std::string password = res[param_index++].value;
                key_password = password;
                modes.insert('k');

                broadcastMode("+k " + password, client);
            }
            else
            {
                key_password.clear();
                modes.erase('k');
                broadcastMode("-k", client);
            }
        }
        
        else if (flag == 'l')
        {
            std::cout << "WITAM W L" << std::endl;
            if (adding)
            {
                std::cout << "ADDING" << std::endl;
                for (size_t i = 0; i < res.size(); ++i)
                    std::cout << i << ": " << res[i].value << std::endl;

                if (param_index >= res.size())
                    return;

                std::string limit_str = res[param_index++].value;

                std::cout << "LIMIT_STR: "<< limit_str << std::endl;

                for (size_t j = 0; j < limit_str.size(); ++j)
                    if (!isdigit(limit_str[j]))
                        return;

                users_limit = atoi(limit_str.c_str());
                modes.insert('l');

                broadcastMode("+l " + limit_str, client);
            }
            else
            {
                users_limit = 0;
                modes.erase('l');
                broadcastMode("-l", client);
            }
        }

        else if (flag == 'o')
        {
            if (param_index >= res.size())
                return;

            std::string nick = res[param_index++].value;

            std::map<int, Client>::iterator it = users.begin();
            for (; it != users.end(); ++it)
            {
                if (it->second.get_nick() == nick)
                {
                    if (adding)
                    {
                        channel_operators.insert(it->first);
                        broadcastMode("+o " + nick, client);
                    }
                    else
                    {
                        channel_operators.erase(it->first);
                        broadcastMode("-o " + nick, client);
                    }
                    break;
                }
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

std::map<int, Client>& Channel::get_users_map()
{
    return users;
}

void Channel::broadcastToChannel(const std::string& msg)
{
    for (std::map<int, Client>::iterator it = users.begin();
         it != users.end(); ++it)
    {
        send(it->first, msg.c_str(), msg.size(), 0);
    }
}

