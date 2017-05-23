CC := gcc
LIBS=ncurses libconfig
CFLAGS=$(shell pkg-config --cflags $(LIBS)) --std=gnu11 -Wall -Wextra -pedantic
LDLIBS=$(shell pkg-config --libs $(LIBS))
OBJECTS := src/error.o src/main.o src/splash.o src/pt.o src/config.o
NAME := alien-console

.PHONY: clean
$(NAME): $(OBJECTS)
	$(CC) -o $(NAME) $(OBJECTS) $(LDLIBS)

clean:
	rm -f $(OBJECTS) $(NAME)

debug: CFLAGS += -DDEBUG -g
debug: $(NAME)

release: CFLAGS += -DRELEASE
release: $(NAME)
