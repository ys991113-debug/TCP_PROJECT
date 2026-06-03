run: client
	stty susp undef; exec ./client 100.83.88.7; stty susp "^Z"
client:
	gcc -Iinclude src/client/client.c -o client
clean:
	rm -f client
