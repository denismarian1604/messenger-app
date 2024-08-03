#### Coded by Denis-Marian Vladulescu

# Basic C Messenger App

## Project description

> **VERSION** : v1.0 update 1

The idea of the project was to build a basic messenger app that would mimic the *Yahoo! Messenger* app. The main features the app should have are:
- Being able to register as a new user
- Being able to log in as an existing user
- Being able to send/accept/decline friend requests
- Being able to talk with multiple friends simultaneously

Besides not having a very pretty interface to work with and not having profile pictures for the users(which may be solved easily on UNIX systems thanks to integrated tools), the project touches all those essential features.

> **NOTE(1)**: The project has been developed completely and only tested on macOS(Sonoma). In order for it to work on linux, the **Terminal.app** app should be replaced with **gnome-terminal**, but this is yet to be tested.

## Data Structures used

**client_t**
> `_client_socket` indicates the client's socket\
> `_info` indicates the client's information\
> `_friends` indicates the client's list of friends which is only a list of clients(I chose the type of the list of friends to be (client_t *) as I wanted to have access to all the informations of the client anywhere)\
> `_nr_friends` indicates the number of clients\
> `_validation_key` indicates the validation key of the user. This may be used as a certificate that proves the user is who it says it is. It can also be seen as a cookie as the password should not be put in each message to the server.

**client_info**
> Used for providing structured information for a user such as its `lastSeen` timestamp, `registered` timestamp, `status`, `username`, `password`, `firstname` and `lastname`.

**friend_req_t**
> Used for representing a friend request between two clients, the `sender` and the `recipient`.

**conversation_t**
> Used for representing a conversation between two users, `_client1` and `_client2`.\
> the `_messages` array is used for storing the exchanged messages between the two users and `_nr_messages` denotes the number of currently stored messages. The default number of messages cap is `15`.

**thread_arg_t**
> Used as the `(void *)` argument given to the functions executed by separate threads.\
> `client_t ***clients` is passed as a triple pointer(reference to an array of `clien_t *`) as one thread or another might update the clients list and that changes need to be reflected and propagated to each and every thread. When a change happens, that current thread acquires the mutex and only frees it when it is done updating the database.\
> `conversation_t ***conversations` is similar to the clients array, as new conversations may appear and they need to be propagated.\
> `_client_socket` is responsible for the socket of the client when we are talking about a client thread.\
> `_variable_variables` / `_variable_pointers` are used for variable variables and/or pointers tha a specific thread might need and other may not.


## Implementation details(server-side)

The main idea of the app is based on having various databases that are loaded into the memory of the server during its initialization. Any information demanded by the clients or any requests are answered from the server's memory and once in a while propagated on the disk.

In order to permanently store the new information from the server's memory on the server's disk, after the initialization of the server has finished, there is a **maintenance thread** launched whose sole purpose is to synchronize the disk information with the server's memory once every 15 seconds(this timer can be easily adjusted). This thread runs completely independed from everything else.

After the maintenance thread is launched, the server listens for new connections. When a client connects to the server, **a new thread** is launched to take care of that client and then the **main thread** resumes waiting for new connections. This allows the server to operate multiple clients simultaneously.

Each thread is then responsible for its own client and listens for **REQUESTS** from it. Depending on the request, the server might only send a succes/failure **RESPONSE** or might also send the demanded informations by the user(conversations/list of friends).

When the client wants to disconnect, its socket and validation code are reset and its `lastSeen` timestamp updated, but the thread does not yet join the main thread as the client might want to reconnect as a different user or maybe reconnect later. Only if the `exit` command is met the client thread will join the main thread.


## Implementation details(client-side)

The client side of the app connects to the server and can then send **REQUESTS** to it. These requests can be but are not limited to register/login/send message/send friend requests.

When the client wants to open a conversation with another user(who *must* be its friend), there are two new binaries launched:
> `conversations_handler`, which is responsible for constantly requesting, parsing and displaying the active conversation between the two users\
> `take_message_input_from_user`, which is responsible for taking input from the keyboard and sending it to the server.


## Application-level communication protocol used explanation

The application-level I used is a personalized HTTP-like protocol.

For authenticating users, as stated before, it uses a `validation key` that is associated with a user when it logs in. The association is removed once the server receives a log out request from that user.

The basic format of a **REQUEST** is :
> REQ/`<operation_code>`*\n*\
> `<specific fields for the request>`*\n*\
> *\n*\
> *\n*

For example, if the user `deadpool` wants to send a friend request to `wolverine` and `deadpool`'s validation code is `567850`, the request will look as follows:

> REQ/6\
> deadpool\
> 567850\
> wolverine\
> ‎ \
> ‎ 

Similarly, a **RESPONSE** from the server goes as follows:
> RES/`<1 for success/ -<someting> for error>`*\n*\
> `<specific fields for the response>`*\n*\
> *\n*\
> *\n*

In the case of the upper presented request, the reponse will look like this(if `deadpool` and `wolverine` were not friends already):

> RES/1\
> Friend request sent successfully.\
> ‎ \
> ‎ 


## Demo screenshots
![Romanian starting menu](<demo_screenshots/Screenshot 2024-07-31 at 20.16.20.png>)
![English starting menu](<demo_screenshots/Screenshot 2024-07-31 at 20.16.34.png>)

![English registration form](<demo_screenshots/Screenshot 2024-07-31 at 20.16.57.png>)

![English logged in menu](<demo_screenshots/Screenshot 2024-07-31 at 20.17.18.png>)

![Conversation opening failure due to not being friends with the recipient](<demo_screenshots/Screenshot 2024-07-31 at 20.19.03.png>)

![Friend requests menu](<demo_screenshots/Screenshot 2024-07-31 at 20.19.14.png>)
![Friend requests menu with one received friend request in romanian](<demo_screenshots/Screenshot 2024-07-31 at 20.19.30.png>)

![Friends list in english](<demo_screenshots/Screenshot 2024-07-31 at 20.19.43.png>)
![Empty conversation between two friends in romanian and english](<demo_screenshots/Screenshot 2024-07-31 at 20.20.20.png>)

![Conversation demo](<demo_screenshots/Screenshot 2024-07-31 at 20.21.07.png>)

![One of the friends is now logged out](<demo_screenshots/Screenshot 2024-07-31 at 20.21.18.png>)

![-](<demo_screenshots/Screenshot 2024-07-31 at 20.21.22.png>)

## App usage

To run the `server`, one would navigate to the `/server_folder` and run the following Makefile command:

> `make clean && make build && make run PORT="desired_port"`

To run one `client` instance, one would navigate to the `/client_folder` and run one of the following Makefile commands:

> `make clean && make build && make run_ro IP="desired_ip" PORT="desired_PORT"` *-- for a romanian-speaking client*\
> `make clean && make build && make run_en IP="desired_ip" PORT="desired_PORT"` *-- for an english-speaking client*

> **NOTE(2)** : In order for the messages to work, the client machines `must` have a `/tmp` folder(a `tmp` folder in the root directory).

> **NOTE(3)** : The present binaries are compiled on `macOS Sonoma Version 14.5, 03/08/2024` on an `M2 Mac`.