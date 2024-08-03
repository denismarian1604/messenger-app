build: clean build_client build_conversations_handler take_message_input_from_user

build_client: client.c client_helper.c client_loader.c
	gcc -Wall -o client $^

build_conversations_handler: conversations_handler.c conversations_handler_helper.c conversation_parser.c
	gcc -Wall -o conversations_handler $^

take_message_input_from_user: take_message_input_from_user.c conversations_handler_helper.c
	gcc -Wall -o take_message_input_from_user $^

run_ro: client
	./client ro $(IP) $(PORT)
run_en: client
	./client en $(IP) $(PORT)

clean:
	rm -f client
	rm -f conversations_handler
