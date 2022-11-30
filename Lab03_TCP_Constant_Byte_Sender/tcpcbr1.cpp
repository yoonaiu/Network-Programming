#include <iostream>
#include <arpa/inet.h> // inet_addr()
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h> // bzero()
#include <sys/socket.h>
#include <unistd.h> // read(), write(), close()
#include <string>
#include <ctime>
#include <algorithm>

#define SA struct sockaddr

using namespace std;

int main(int argc, char* argv[]){

    string rate_str = argv[1];
    float rate = stof(rate_str);

    // 1. socket
    int socket_fd;
    socket_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if(socket_fd == 0){
        cout << "socket creation fail" << endl;
        exit(0);
    }else{
        cout << "socket creation success" << endl;
    }

    // 2. connect
    struct sockaddr_in servaddr;
    bzero(&servaddr, sizeof(servaddr));

    // assign IP, PORT
    servaddr.sin_family = AF_INET;
    // servaddr.sin_addr.s_addr = inet_addr("140.113.213.213");
    servaddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    servaddr.sin_port = htons(10003);

    if (connect(socket_fd, (SA*)&servaddr, sizeof(servaddr)) != 0) {
        cout << "connection with the server failed..." << endl;
    }else{
        cout << "connection with the server success" << endl;
    }

    // 3. write in constant rate
    rate = 1000000*rate;
    string send_amount = "";
    for(int i = 0; i < rate; i++){
        send_amount += "s";  // one char in string is 1 byte
    }

    char *send = (char*)calloc(rate, sizeof(char));
    send = send_amount.data();

    int write_success;
    time_t start_time;
    
    while(true){
        start_time = time(NULL);

        // if((time(NULL) - total_start_time) > 10) break;
        write_success = write(socket_fd, send, send_amount.length());
        // cout << write_success << endl;
        cout << "send at " << ((float)write_success)/1000000 << " MBps" << endl;

        sleep( 1 - (time(NULL) - start_time) );        
    }

    close(socket_fd);

    return 0;
}