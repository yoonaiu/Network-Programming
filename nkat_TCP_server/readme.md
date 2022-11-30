# Reference
* Practice from NYCU NP 111 course lab04: https://md.zoolab.org/s/XwIoxShTM

# Introduction
1. Run the designated service to be network service on the localhost with designated port number
2. Related to file descriptor allocating

# Usage
Command in Terminal:
```./nkat <port> <path to external program>```
Means that run the external program on the localhost:<\port> to be a network service and can be accessed by ```nc localhost <\port>```

For example, after deploying ```./nkat 11111 date```, we can use ```nc localhost 11111``` to access the 'date' service on localhost:11111.