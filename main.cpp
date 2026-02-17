#include "Server.hpp"
#include "Commands.hpp"
#include <csignal>
#include <cerrno>

void signalHandler(int)
{
}

int main(int argc, char **argv)
{
    // Rejestracja SIGINT
    signal(SIGINT, signalHandler);

    Server vars;
    struct sockaddr_in my_addr;
    if (argc != 3)
        return -1;
    std::string port = argv[1];
    if (!(port.find_first_not_of("0123456789") == std::string::npos))
    {
        std::cout << "Invalid port\n";
        return -1;
    }
    if (vars.SetSocketFd() < 0)
    {
        std::cout << "socket Error";
        return 1;
    }
    std::string pass = argv[2];
    vars.SetPassword(pass);
    my_addr.sin_family = AF_INET;
    my_addr.sin_port = htons(std::atoi(port.c_str()));
    my_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    set_nonblocking(vars.GetSocketFd());

    int opt = 1;
    if (setsockopt(vars.GetSocketFd(), SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(int)) < 0)
    {
        perror("setsockopt");
        return 1;
    }

    if (bind(vars.GetSocketFd(), (struct sockaddr *)&my_addr, sizeof(my_addr)) < 0)
        return perror("bind"), 1;
    if (listen(vars.GetSocketFd(), SOMAXCONN) < 0)
        return perror("listen"), 1;

    std::cout << "IRC server listening on port " << port << std::endl;
    while (1)
    {
        FD_ZERO(&vars.GetReadFds());
        FD_SET(vars.GetSocketFd(), &vars.GetReadFds());
        vars.SetMaxFd(vars.GetSocketFd());
        vars.SetClientRead();
        int activity = select(vars.GetMaxFd() + 1, &vars.GetReadFds(), NULL, NULL, NULL);
        if (activity < 0)
        {
            if (errno == EINTR) // EINTR = Interrupted system call -> dostajemy po ctrl + C
            {
                std::cout << "\nSIGINT received. Shutting down...\n";
                break;
            }
            perror("select");
            continue;
        }
        vars.AcceptClient();
        vars.ClientHandle();
    }
    close(vars.GetSocketFd());

    std::cout << "Server closed properly.\n";
    return 0;
}
