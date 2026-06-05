PI_USER ?= lys
PI_HOST ?= 100.83.88.7
PI_DIR  ?= /home/lys/project
CC_ARM  = aarch64-linux-gnu-gcc
CC      = gcc
CFLAGS  = -Iinclude -Icross/include
LFLAGS  = -Lcross/lib -lwiringPi -Wl,-rpath,/usr/lib -Wl,--allow-shlib-undefined -Wl,--unresolved-symbols=ignore-in-shared-libs
DEVICES = led light seg buzzer
SOS     = $(addprefix cross/libdev_,$(addsuffix .so,$(DEVICES)))

.PHONY: all cross client web clean run deploy stop

all: cross client web deploy

cross: $(SOS) cross/server cross/webserver

cross/libdev_%.so: src/devices/%.c
	  $(CC_ARM) -shared -fPIC $(CFLAGS) $< -o $@ $(LFLAGS) -lpthread

cross/server: src/server/main.c src/server/proto.c src/server/loader.c
	$(CC_ARM) $(CFLAGS) src/server/main.c src/server/proto.c src/server/loader.c -o $@ $(LFLAGS) -lpthread -ldl

client: src/client/client.c
	  $(CC) -Iinclude $< -o client

web: web/webserver.c
	  $(CC) -Wall $< -o web/webserver

cross/webserver: web/webserver.c
	$(CC_ARM) $< -o cross/webserver

deploy: cross
	ssh $(PI_USER)@$(PI_HOST) "sudo fuser -k 1833/tcp 8080/tcp 2>/dev/null; mkdir -p $(PI_DIR)/lib $(PI_DIR)/web"
		scp cross/server $(PI_USER)@$(PI_HOST):$(PI_DIR)/
	scp cross/libdev_*.so $(PI_USER)@$(PI_HOST):$(PI_DIR)/lib/
	scp cross/webserver $(PI_USER)@$(PI_HOST):$(PI_DIR)/
	scp web/index.html $(PI_USER)@$(PI_HOST):$(PI_DIR)/web/

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
	  rm -f client web/webserver cross/server cross/webserver cross/libdev_*.so