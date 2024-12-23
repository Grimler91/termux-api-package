PREFIX ?= /data/data/com.termux/files/usr
CFLAGS += -std=c11 -Wall -Wextra -pedantic -Werror -DPREFIX=\"$(PREFIX)\"
CC ?= gcc
AR ?= ar

termux-api.o: termux-api.c
	$(CC) -c $(CFLAGS) -o termux-api.o termux-api.c

termux-api: termux-api.o
	$(CC) -shared -o libtermux-api.so termux-api.o

termux-api-static: termux-api.o
	$(AR) r libtermux-api.a termux-api.o 

termux-api-broadcast: termux-api-broadcast.c termux-api
	$(CC) -ltermux-api -o termux-api-broadcast

install:
	mkdir -p $(PREFIX)/bin/ $(PREFIX)/libexec/
	cd scripts; for i in scripts/*; do \
		sed -e "s|@TERMUX_PREFIX@|$(PREFIX)|g" $$i > $(PREFIX)/bin/$$i; \
		chmod 700 $(PREFIX)/bin/$$i; \
	done

	install -Dm700 termux-api.h $(PREFIX)/include
	install -Dm700 libtermux-api.so $(PREFIX)/lib
	install -Dm700 libtermux-api.a $(PREFIX)/lib

	sed -e "s|@TERMUX_PREFIX@|$(PREFIX)|g" termux-callback.in > $(PREFIX)/libexec/termux-callback
	chmod 755 $(PREFIX)/libexec/termux-callback
	install -Dm755 termux-api-broadcast $(PREFIX)/libexec/termux-api-broadcast
	ln -s termux-api-broadcast $(PREFIX)/libexec/termux-api

.PHONY: install
