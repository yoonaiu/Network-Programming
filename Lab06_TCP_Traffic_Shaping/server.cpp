// #define FD_SETSIZE 3000
#include <stdio.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdlib.h>/* 亂數相關函數 */
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
#include <time.h> /* 時間相關函數 */
#include <map>
#include "function.h"
#include <sys/time.h>
#define MAX 80
#define MAXLINE 10000
#define SA struct sockaddr
#define LISTENQ 1024
using namespace std;

class client_info {
public:
	string name;
	int connfd;
	struct sockaddr_in connect_info;

	client_info(string name_in, int connfd_in, struct sockaddr_in connect_info_in){
		name = name_in;
		connfd = connfd_in;
		connect_info = connect_info_in;
	}

    client_info(){
        name = "";
        connfd = -1;
    } // map[i] 沒東西的話會 call 到這個，所以這個沒有的時候編譯編不過
};

void broadcast(string msg, int client[FD_SETSIZE], int maxi, int skip, fd_set rset, fd_set allset, int maxfd){
    for(int i = 0; i <= maxi; i++){
        if(client[i] == -1 || i == skip) continue;
        send(client[i], msg.c_str(), msg.length(), MSG_NOSIGNAL);  // 
    }
}

double get_time_double(struct timeval *tv){
  string time_str = to_string(tv->tv_sec) + "." + to_string(tv->tv_usec);
  double time = stod(time_str);
  return time;
}


int main(int argc, char **argv) {

    // 1. command channel
    int					command_i, command_maxi, command_maxfd, command_listenfd, command_connfd, command_sockfd;
	int					command_nready, command_client[FD_SETSIZE];  // FD_SETSIZE is 1024, can handle 1000 client
	ssize_t				command_n;
	fd_set				command_rset, command_allset;
	char				command_buf[MAXLINE];
	socklen_t			command_clilen;
	struct sockaddr_in	command_cliaddr, command_servaddr;
    uint16_t command_SERV_PORT = atoi(argv[1]);
    command_listenfd = Socket(AF_INET, SOCK_STREAM, 0);

	bzero(&command_servaddr, sizeof(command_servaddr));
	command_servaddr.sin_family      = AF_INET;
	command_servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	command_servaddr.sin_port        = htons(command_SERV_PORT);

    Bind(command_listenfd, (SA *) &command_servaddr, sizeof(command_servaddr));
    cerr << "Port " << command_SERV_PORT << " bind success" << endl;
    Listen(command_listenfd, LISTENQ);

	command_maxfd = command_listenfd;			/* initialize */
	command_maxi = -1;					/* index into client[] array */
	for (command_i = 0; command_i < FD_SETSIZE; command_i++)
		command_client[command_i] = -1;			/* -1 indicates available entry */
	FD_ZERO(&command_allset);
	FD_SET(command_listenfd, &command_allset);

    // 2. sink channel
    int					sink_i, sink_maxi, sink_maxfd, sink_listenfd, sink_connfd, sink_sockfd;
	int					sink_nready, sink_client[FD_SETSIZE];  // FD_SETSIZE is 1024, can handle 1000 client
	ssize_t				sink_n;
	fd_set				sink_rset, sink_allset;
	char				sink_buf[MAXLINE];
	socklen_t			sink_clilen;
	struct sockaddr_in	sink_cliaddr, sink_servaddr;
    uint16_t sink_SERV_PORT = atoi(argv[1]) + 1;
    sink_listenfd = Socket(AF_INET, SOCK_STREAM, 0);  // set the sink channel flag to non blocking
	int flags = fcntl(sink_listenfd, F_GETFL, 0);
	fcntl(sink_listenfd, F_SETFL, flags | O_NONBLOCK);


	bzero(&sink_servaddr, sizeof(sink_servaddr));
	sink_servaddr.sin_family      = AF_INET;
	sink_servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	sink_servaddr.sin_port        = htons(sink_SERV_PORT);

    cerr << "Port " << sink_SERV_PORT << " not yet bind" << endl;
    Bind(sink_listenfd, (SA *) &sink_servaddr, sizeof(sink_servaddr));
    cerr << "Port " << sink_SERV_PORT << " bind success" << endl;
    Listen(sink_listenfd, LISTENQ);

	sink_maxfd = sink_listenfd;			/* initialize */
	sink_maxi = -1;					/* index into client[] array */
	for (sink_i = 0; sink_i < FD_SETSIZE; sink_i++)
		sink_client[sink_i] = -1;			/* -1 indicates available entry */
	FD_ZERO(&sink_allset);
	FD_SET(sink_listenfd, &sink_allset);
    int receive_bytes = 0;

	// both
	// set up for the select function -> 一點都不想等
    struct timeval waiting_time;
	waiting_time.tv_sec = 0;
	waiting_time.tv_usec = 0;

	struct timeval sink_waiting_time;
	waiting_time.tv_sec = 0;
	waiting_time.tv_usec = 100000;

	// set to be the time we initialize it
	struct timeval time_cnt_start;
	gettimeofday(&time_cnt_start, NULL);


    for( ; ; ){

        // 2. sink channel second
        sink_rset = sink_allset;		/* structure assignment */
		sink_nready = select(sink_maxfd+1, &sink_rset, NULL, NULL, &sink_waiting_time);


		if (FD_ISSET(sink_listenfd, &sink_rset)) {	/* new client connection */
			cout << "sink clients -------------- 1 !!\n";
			
			// need to accept multiple client a time 
			for(int k = 0; k < 1024; k++) {
				sink_clilen = sizeof(sink_cliaddr);
				int sink_connfd = accept(sink_listenfd, (SA *) &sink_cliaddr, &sink_clilen);  // 一次只會接一個 client 近來
				cout << "sink_connfd: " << sink_connfd << endl;
				if(sink_connfd == -1) break;  // 如果沒有 client 了 -> break
				cout << "sink clients -------------- 2 !!\n";

#ifdef	NOTDEF
			printf("new client: %s, port %d\n",
					Inet_ntop(AF_INET, &cliaddr.sin_addr, 4, NULL),
					ntohs(cliaddr.sin_port));
#endif
				for (sink_i = 0; sink_i < FD_SETSIZE; sink_i++){
					if (sink_client[sink_i] < 0) {
						sink_client[sink_i] = sink_connfd;	/* save descriptor */
						// ** 1 ** Welcome msg
						cout << "sink clients -------------- 3 !!\n";
						break;
					}
				}

				if (sink_i == FD_SETSIZE){
					cerr << "too many clients" << endl;
					exit(0);
				} // may equal to -> err_quit("too many clients");

				FD_SET(sink_connfd, &sink_allset);	/* add new descriptor to set */
				if (sink_connfd > sink_maxfd)
					sink_maxfd = sink_connfd;			/* for select */
				if (sink_i > sink_maxi)
					sink_maxi = sink_i;				/* max index in client[] array */

				if (--sink_nready <= 0)
					continue;				/* no more readable descriptors */ // -> 處理好一個人了，如果只有他那不用跑下面的 for

			}
			cout << "sink clients 4 -------------- !!\n";

// #ifdef	NOTDEF
// 			printf("new client: %s, port %d\n",
// 					Inet_ntop(AF_INET, &cliaddr.sin_addr, 4, NULL),
// 					ntohs(cliaddr.sin_port));
// #endif
		}

        // PART2 - handle old connection -> need disconnect / search info
		for (sink_i = 0; sink_i <= sink_maxi; sink_i++) {	/* check all clients for data */
			if ( (sink_sockfd = sink_client[sink_i]) < 0)
				continue;
			if (FD_ISSET(sink_sockfd, &sink_rset)) { // check descriptor is ready -> leave, (broken), connect to host
				int nn;
				memset(sink_buf, 0, sizeof(sink_buf));
				nn = read(sink_sockfd, sink_buf, MAXLINE);
                
                receive_bytes += nn; // add here

				if (nn == 0) {  // someone leave -> collect the broadcast message and send to all user still online after the for loop
					/*4connection closed by client */
					close(sink_sockfd);
					FD_CLR(sink_sockfd, &sink_allset);
					cout << "nn == 0 sink_sockfd: " << sink_sockfd << endl;
					sink_client[sink_i] = -1;  //**** don't remove the client!!
                    
                    // -> change to remember the leave client and send to all

				} else if (nn == -1){
                    // 還會 read fail 可能是因為斷的比上面處理得還快，所以來不及標記 client[i] 為 -1
                    // 所以這邊還會 read 到 client[i] 所以會出錯
					cout << "read fail" << endl;
                    // cerr << strerror(errno) << endl;
                    char error_buf[256];
                    memset(error_buf, 0, sizeof(error_buf));
                    perror(error_buf); // print the error msg directly to the stderr
					// exit(0);  // **** broken here ****
				} else {
                    string k; // just do something

				}

				if (--sink_nready <= 0)
					break;				/* no more readable descriptors */
			}
		}
		// sleep(1);

		// 1. command channel first
        // ---- start -----
        command_rset = command_allset;		/* structure assignment */
		command_nready = select(command_maxfd+1, &command_rset, NULL, NULL, &waiting_time);

		if (FD_ISSET(command_listenfd, &command_rset)) {	/* new client connection */
			command_clilen = sizeof(command_cliaddr);
            int command_connfd = Accept(command_listenfd, (SA *) &command_cliaddr, &command_clilen);
#ifdef	NOTDEF
			printf("new client: %s, port %d\n",
					Inet_ntop(AF_INET, &cliaddr.sin_addr, 4, NULL),
					ntohs(cliaddr.sin_port));
#endif
			for (command_i = 0; command_i < FD_SETSIZE; command_i++){
				if (command_client[command_i] < 0) {
					command_client[command_i] = command_connfd;	/* save descriptor */
					// ** 1 ** Welcome msg
					break;
				}
            }

			if (command_i == FD_SETSIZE){
				cerr << "too many clients" << endl;
				exit(0);
			} // may equal to -> err_quit("too many clients");

			FD_SET(command_connfd, &command_allset);	/* add new descriptor to set */
			if (command_connfd > command_maxfd)
				command_maxfd = command_connfd;			/* for select */
			if (command_i > command_maxi)
				command_maxi = command_i;				/* max index in client[] array */

			if (--command_nready <= 0)
				continue;				/* no more readable descriptors */ // -> 處理好一個人了，如果只有他那不用跑下面的 for
		}

        // PART2 - handle old connection -> need disconnect / search info
		for (command_i = 0; command_i <= command_maxi; command_i++) {	/* check all clients for data */
			if ( (command_sockfd = command_client[command_i]) < 0)
				continue;
			if (FD_ISSET(command_sockfd, &command_rset)) { // check descriptor is ready -> leave, (broken), connect to host
				int n;
				memset(command_buf, 0, sizeof(command_buf));
				n = read(command_sockfd, command_buf, MAXLINE);

				if (n == 0) {  // someone leave -> collect the broadcast message and send to all user still online after the for loop
					/*4connection closed by client */
					close(command_sockfd);
					FD_CLR(command_sockfd, &command_allset);
					command_client[command_i] = -1;
                    
                    // -> change to remember the leave client and send to all

				} else if (n == -1){
                    // 還會 read fail 可能是因為斷的比上面處理得還快，所以來不及標記 client[i] 為 -1
                    // 所以這邊還會 read 到 client[i] 所以會出錯
					cout << "read fail" << endl;
                    // cerr << strerror(errno) << endl;
                    char error_buf[256];
                    memset(error_buf, 0, sizeof(error_buf));
                    perror(error_buf); // print the error msg directly to the stderr
					// exit(0);  // **** broken here ****
				} else {
					string str_user_input = command_buf;
					// cout << "user input: " << str_user_input;

                    if( str_user_input.substr(0, 5) == "/ping"){

                        struct timeval tv;
                        gettimeofday(&tv, NULL);
                        // cout << tv.tv_sec << "." << tv.tv_usec << endl;
                        string to_user = to_string(tv.tv_sec) + "." + to_string(tv.tv_usec) + " PONG" + "\n";
                        Write(command_sockfd, to_user.c_str(), to_user.length());
                    }
    
                    if( str_user_input.substr(0, 8) == "/clients"){
						cout << "get clients @@@@ " << endl;
                        int client_num = 0;
                        for(int cnt = 0; cnt < FD_SETSIZE; cnt++){
                            if(sink_client[cnt] != -1){
								client_num++;
								cout << "client_num++: " << cnt << endl; 
							} 
							
                        }
                        struct timeval tv;
                        gettimeofday(&tv, NULL);
                        // <time> CLIENTS <number-of-connected-data-sink-connections>\n
                        string to_user = to_string(tv.tv_sec) + "." + to_string(tv.tv_usec) + " CLIENTS " + to_string(client_num) + "\n";
                        Write(command_sockfd, to_user.c_str(), to_user.length());
                    }

                    if( str_user_input.substr(0, 7) == "/report"){
                        struct timeval tv;
                        gettimeofday(&tv, NULL);
						double elapsed_seconds = get_time_double(&tv) - get_time_double(&time_cnt_start);
						// long elapsed_seconds = tv.tv_sec - time_cnt_start.tv_sec;
						double measured_megabits_per_second = (8.0 * double(receive_bytes) / double(1e6)) / double(elapsed_seconds);  // 這邊只拿 second 來除

                        // <time> REPORT <counter-value> <elapsed-time>s <measured-megabits-per-second>Mbps\n
                        string to_user = to_string(tv.tv_sec) + "." + to_string(tv.tv_usec) 
									   + " REPORT " + to_string(receive_bytes) + " "
									   + to_string(elapsed_seconds) + "s "
									   + to_string(measured_megabits_per_second) + "Mbps" + "\n";
                        
                        Write(command_sockfd, to_user.c_str(), to_user.length());
                    }

                    if( str_user_input.substr(0, 6) == "/reset"){
                        struct timeval tv;
                        gettimeofday(&tv, NULL);
                        // <time> CLIENTS <number-of-connected-data-sink-connections>\n
                        string to_user = to_string(tv.tv_sec) + "." + to_string(tv.tv_usec) + " RESET " + to_string(receive_bytes) + "\n";
                        Write(command_sockfd, to_user.c_str(), to_user.length());
                        receive_bytes = 0;
						gettimeofday(&time_cnt_start, NULL);
                    }

				}

				if (--command_nready <= 0)
					break;				/* no more readable descriptors */
			}
		}
        // ---- end   -----
    }
}
