FILES = main \
		Client \
		Server \
		Commands \
		Channel

COMPILER = c++

FLAGS = -Wall -Wextra -Werror -std=c++98

CFILES = $(FILES:%=%.cpp)

OFILES = $(FILES:%=%.o)

NAME	= ircserv

$(NAME):
			$(COMPILER) $(FLAGS) $(CFILES) -o $(NAME)

all: $(NAME)

clean:
	rm -f $(NAME)

fclean: clean

re: fclean all

.PHONY: all, clean, fclean, re