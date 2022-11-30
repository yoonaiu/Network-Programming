# Reference
* Practice from NYCU NP 111 course lab06: https://md.zoolab.org/s/fGzkg8D79

# Introduction
This lab including a server and a client.

Server:
1. Run at designated <\port> and <\port+1>
2. <\port> receiveing command msg
3. <\port+1> receiving any text, like a sink
4. The server will count the byte number receiving from the <\port+1> and response the corressponding inquiry/operation msg send to the <\port>

Client:
1. Send data to the server and achieve above 85% sending rate even with harsh delay demanding by the test2.sh
2. Achieving by building up multiple connections to the server to achieve high sending rate even with the delay

# Usage
Command in Terminal:
1. to run up server ```./server <port>```
2. connect to the server by nc ```nc localhost <port>``` or ```nc localhost <port+1>```
3. test the "server" with test script with: ```./test1.sh``` after running up the server
4. (after running up the server on the corresponding port, you can) test the "client" with test script with: ```./test2.sh```, this test script will automatically run up the client and show its performance of sending rate


Command in Server <port>:
1. ```/reset```: reset the byte count of <\port+1> and server will respond how many bytes have been received from all data sink connections to the <\port+1>
2. ```/ping```: ping the server, server will respond server's liveness
3. ```/report```: get current counter value in the server, with megabits-per-second format
4. ```/clients```: reports how many data sink connections to server's <\port+1> is currently saved by the server
