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
#define SA struct sockaddr

using namespace std;

/*
socket
connect
write
read
close

*/

int main(){
    
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
    servaddr.sin_addr.s_addr = inet_addr("140.113.213.213");
    servaddr.sin_port = htons(10002);

    if (connect(socket_fd, (SA*)&servaddr, sizeof(servaddr)) != 0) {
        cout << "connection with the server failed..." << endl;
    }else{
        cout << "connection with the server success" << endl;
    }

    // write
    string tmp3 = "GO\n";
    char buf[] = "GO\n";
    int write_success;
    write_success = write(socket_fd, buf, tmp3.length());
    if(write_success == -1){
        cout << "write error" << endl;
    }else{
        cout << "write success" << endl;
    }
    sleep(1);

    // read
    char readbuf[50000];
    memset( readbuf, '\0', sizeof(char)*50000 );
    int read_return;
    string result = "";
    string total_result = "";
    // long long sum;
    int sum;
    sum = 0;
    int len;

    while(true){
        read_return = read(socket_fd, readbuf, sizeof(readbuf));
        result = std::string(readbuf);
        if(result.find("==== BEGIN DATA ====\n") != std::string::npos){
            sum = 0;
            string tmp = "==== BEGIN DATA ====\n";
            sum += 50000 - (result.find("==== BEGIN DATA ====\n") + tmp.length());
        }

        total_result += result;

        memset( readbuf, '\0', sizeof(char)*50000 );
        if(result.find("=== END DATA ====") != std::string::npos){
            sum += result.find("=== END DATA ====");
            break;
        }
    }
    cout << "hello" << endl;
    cout << sum << endl;
    cout << total_result.find("==== BEGIN DATA ====") << endl << total_result.find("==== END DATA ====") << endl;
    cout << "hello" << endl;
    int final_ans;
    final_ans = total_result.find("==== END DATA ====") - total_result.find("==== BEGIN DATA ====") - 22;
    cout << final_ans << endl;

    sleep(1);


    string answer = std::to_string(final_ans);
    answer += "\n";

    cout << answer << endl;
    char *an = (char*)calloc(answer.length(), sizeof(char));

    an = answer.data();
    write_success = write(socket_fd, an, answer.length());
    cout << write_success << endl;

    char readresult[500];
    memset( readresult, '\0', sizeof(char)*500 );
    read_return = read(socket_fd, readresult, sizeof(readresult));
    cout << readresult << endl;

    close(socket_fd);

    return 0;
}