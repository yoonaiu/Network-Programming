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
#include "function.h"
#define SA struct sockaddr
#define CLIENTNUM 20

using namespace std;


// COMMAND variable - 9998
int reset_socket_fd;
struct sockaddr_in reset_servaddr;


// SINK variable - 9999
int socket_fd[CLIENTNUM];
// int socket_fd1;


void signalHandler( int signum )
{
    cout << "Interrupt signal (" << signum << ") received.\n";
    // 清理并关闭
    // 终止程序
    // 1. write /report command and read back the msg from the server
    string reset = "/report\n";
    Write(reset_socket_fd, reset.c_str(), reset.length());

	char				reset_buf[MAXLINE];
    int nn;
    memset(reset_buf, 0, sizeof(reset_buf));
    nn = read(reset_socket_fd, reset_buf, MAXLINE);
    cout << reset_buf;

    // 2. close 9999, 9998
    close(reset_socket_fd);
    for(int i = 0; i < CLIENTNUM; i++) {
        close(socket_fd[i]);
    }

    exit(signum);  
}

int main(int argc, char* argv[]){

    // 沒有第四個參數 $3，應該就是狂送猛送
    // 0: ./client
    // 1: 127.0.0.1
    // 2: 9998

    // PART 1 - send /reset to 9998
    reset_socket_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if(reset_socket_fd == 0){
        cout << "socket creation fail" << endl;
        exit(0);
    }

    bzero(&reset_servaddr, sizeof(reset_servaddr));
    reset_servaddr.sin_family = AF_INET;
    reset_servaddr.sin_addr.s_addr = inet_addr(argv[1]);
    reset_servaddr.sin_port = htons(atoi(argv[2]));

    if (connect(reset_socket_fd, (SA*)&reset_servaddr, sizeof(reset_servaddr)) != 0) {
        cout << "connection with the server failed..." << endl;
    }

    string reset = "/reset\n";
    Write(reset_socket_fd, reset.c_str(), reset.length());

	char				reset_buf[MAXLINE];
    int nn;
    memset(reset_buf, 0, sizeof(reset_buf));
    nn = read(reset_socket_fd, reset_buf, MAXLINE);
    cout << reset_buf;


    for(int i = 0; i < CLIENTNUM; i++){
        socket_fd[i] = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if(socket_fd[i] == 0){
            cout << "socket creation fail" << endl;
            exit(0);
        }
        // 2. connect
        struct sockaddr_in servaddr;
        bzero(&servaddr, sizeof(servaddr));

        servaddr.sin_family = AF_INET;
        servaddr.sin_addr.s_addr = inet_addr(argv[1]);
        servaddr.sin_port = htons(atoi(argv[2]) + 1);

        if (connect(socket_fd[i], (SA*)&servaddr, sizeof(servaddr)) != 0) {
            cout << "connection with the server failed..." << endl;
        }

    }

    // PART 3 - when receive terminate signal
    // 注册信号 SIGINT 和信号处理程序
    signal(SIGTERM, signalHandler); 

    // 3. write in constant rate
    long rate = 1e5;  // 總共會送 40 萬過去
    string send_amount = "";
    for(long i = 0; i < rate; i++){
        send_amount += "s";
    }

    char *send = (char*)calloc(rate, sizeof(char));
    send = send_amount.data();

    int write_success;
    while(true){
        for(int i = 0; i < CLIENTNUM; i++) {
            write_success = write(socket_fd[i], send, send_amount.length());
        }
    }

    return 0;
}