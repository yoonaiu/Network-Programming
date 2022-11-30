#include <stdio.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/signal.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdint.h>
#include <arpa/inet.h>
#include <errno.h>
#include <iostream>
#include <cstdint> 
#include <string.h>
// #include stdint.h
#define MAX 80
// #define PORT 11111
#define SA struct sockaddr
using namespace std;

void sig_chld(int signo) {
    pid_t pid;
    int stat;
    // pid = wait(&stat);
    while((pid == waitpid(-1, &stat, WNOHANG)) > 0)  // wait for all child process
        printf("child %d terminate\n", pid);

    return;
}

int main(int argc, char **argv)
{
	int sockfd, connfd;
    pid_t childpid;
    socklen_t clilen;
	struct sockaddr_in servaddr, cliaddr;
    void sig_child(int);
    uint16_t PORT = atoi(argv[1]);  // stdlib.h

    // 1. socket
	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		printf("Socket creation failed...\n");
		exit(0);
	} else {
		printf("Socket successfully created..\n");
    }
	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr.sin_port = htons(PORT);

	// 2. bind
	if ((bind(sockfd, (SA*)&servaddr, sizeof(servaddr))) != 0) {
		printf("Socket bind failed...\n");
		exit(0);
	} else {
		printf("Socket successfully binded..\n");
    }

	// 3. listen
	if ((listen(sockfd, 5)) != 0) {
		printf("Listen failed...\n");
		exit(0);
	} else {
		printf("Server listening..\n");
    }

    // 4. parent - setup prevent zombie process
    signal(SIGCHLD, sig_chld);	/* must call waitpid() */

    for(; ;) {
        int pipefd[2];
        pipe(pipefd);

        clilen = sizeof(cliaddr);

        // 4. accept
        if ( (connfd = accept(sockfd, (SA*) &cliaddr, &clilen)) < 0) {  // if error
            if(errno == EINTR) continue;
            else {
                printf("Server accept failed...\n");
		        exit(0);
            }
        }

        // print client information
        // reference: http://www.samirchen.com/get-client-server-ip-port/
        // 應該沒錯
        struct sockaddr_in client;
        socklen_t clientsz = sizeof(client);
        getsockname(connfd, (struct sockaddr *) &client, &clientsz);
        cout << "New connection from " << inet_ntoa(client.sin_addr) << ":" << ntohs(client.sin_port) << endl;

        // success
        if((childpid = fork()) == 0) {
            
            // close child's socket -> child do not need the original sockfd
            // now child only need the descriptor generate after accept to communicate with client
            close(sockfd);

            dup2(connfd, STDOUT_FILENO);  // use connfd 蓋掉 STDOUT
            dup2(connfd, STDIN_FILENO);  // use connfd 蓋掉 STDIN
            // *** client 那邊下指令後出錯，要把 stderr 接去 client 那邊顯示 ***
            int stderr_fd;
            stderr_fd = dup(STDERR_FILENO); // move stderr to client
            dup2(connfd, STDERR_FILENO);  // use connfd 蓋掉 STDERR

            if(execvp(argv[2], argv+2) == -1){  // stderr in execvp remains the same -> already correct
                // do not show things to client -> use stderr to show things
                // *** 這邊是 server 自己的錯，在 server 這邊 output *** -> Q3
                dup2(stderr_fd, STDERR_FILENO);  // move back the stderr to server
                // use stderr_fd 蓋掉 STDERR_FILENO

                char error_buf[256];
                memset(error_buf, 0, sizeof(error_buf));
                cerr << "Child process terminated, error message: ";
                perror(error_buf); // print the error msg directly to the stderr

                exit(0);
            }
        }
        close(connfd);
    }

    return 0;
}
