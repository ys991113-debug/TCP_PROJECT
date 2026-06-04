PI_USER ?= lys
PI_HOST ?= 100.83.88.7
PI_DIR  ?= /home/lys/project
CC_ARM  = aarch64-linux-gnu-gcc
CC      = gcc
CFLAGS  = -Iinclude -Icross/include
LFLAGS  = -Lcross/lib -lwiringPi -Wl,-rpath,/usr/lib
DEVICES = led light seg buzzer
SOS     = $(addprefix cross/libdev_,$(addsuffix .so,$(DEVICES)))

.PHONY: all cross client web clean run

all: cross client web

cross: $(SOS) cross/server

cross/libdev_%.so: src/devices/%.c
	  $(CC_ARM) -shared -fPIC $(CFLAGS) $< -o $@ $(LFLAGS) -lpthread

cross/server: src/server/main.c src/server/proto.c src/server/loader.c
	  $(CC_ARM) $(CFLAGS) $^ -o $@ $(LFLAGS) -lpthread -ldl

client: src/client/client.c
	  $(CC) -Iinclude $< -o client

web: web/webserver.c
	  $(CC) -Wall $< -o web/webserver

stop:
	-ssh $(PI_USER)@$(PI_HOST) "sudo kill \$$(cat /tmp/tcpserver.pid) 2>/dev/null"

run: client web
	web/webserver $(PI_HOST) & \
	sleep 1 && \
	xdg-open http://localhost:8080/ & \
	sleep 1 && \
	stty susp undef && ./client $(PI_HOST); \
	stty susp "^Z"; \
	kill %1 2>/dev/null
clean:
	  rm -f client web/webserver cross/server cross/libdev_*.so