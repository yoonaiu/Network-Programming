# Reference
* Practice from NYCU NP 111 course lab05: https://md.zoolab.org/s/Heyqdw2Uj

# Introduction
1. Simple TCP chat server. Multiple clients can connect to it at the same time and chat with each others.
2. User will receive message from the server system following the format listed here: https://md.zoolab.org/s/Heyqdw2Uj
3. Handle multiple clients with single process I/O multiplexing.
4. Affordable for 1000 clients connect and disconnect at the same time.

# Usage
Command in Terminal:
1. run up the server: ```./server <port>```
2. connect to the server: ```nc localhost <port>```

Command in Chat Server:
1. ```/name <nickname>```: set the nick name for the current user
2. ```/who```: list all online users with nickname, ip, port
3. simply type text message will be the chat room message
