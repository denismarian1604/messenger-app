build: server.c server_helper.c loader.c messages_handler.c
	gcc -Wall -o server server.c server_helper.c loader.c messages_handler.c

run: server
	./server ../configs/config_ro.txt ../databases/clients_db.txt ../databases/friends_db.txt $(PORT)

clean:
	rm -f server
