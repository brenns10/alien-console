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

install: $(NAME)
	install -Dvm755 $(NAME) /usr/bin/alien-console
	install -Dvm644 etc/alien-console.conf /usr/share/alien-console/alien-console.conf
	install -Dvm644 etc/eg0.txt /usr/share/alien-console/eg0.txt
	install -Dvm644 etc/eg1.txt /usr/share/alien-console/eg1.txt
	install -Dvm644 etc/eg2.txt /usr/share/alien-console/eg2.txt
	install -Dvm644 etc/eg3.txt /usr/share/alien-console/eg3.txt
	install -Dvm644 etc/splash.txt /usr/share/alien-console/splash.txt
	install -Dvm644 etc/CHANGELOG.md /usr/share/alien-console/CHANGELOG.md

uninstall:
	rm -f /usr/bin/alien-console
	rm -f /usr/share/alien-console/alien-console.conf
	rm -f /usr/share/alien-console/eg0.txt
	rm -f /usr/share/alien-console/eg1.txt
	rm -f /usr/share/alien-console/eg2.txt
	rm -f /usr/share/alien-console/eg3.txt
	rm -f /usr/share/alien-console/splash.txt
	rm -f /usr/share/alien-console/CHANGELOG.md
	rmdir /usr/share/alien-console
