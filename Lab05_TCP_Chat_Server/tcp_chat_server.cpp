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

int main(int argc, char **argv) {
	int					i, maxi, maxfd, listenfd, connfd, sockfd;
	int					nready, client[FD_SETSIZE];  // FD_SETSIZE is 1024, can handle 1000 client
	ssize_t				n;
	fd_set				rset, allset;
	char				buf[MAXLINE];
	socklen_t			clilen;
	struct sockaddr_in	cliaddr, servaddr;
    uint16_t SERV_PORT = atoi(argv[1]);
	map<int, client_info> client_map; // index in client array <-> client info

	listenfd = Socket(AF_INET, SOCK_STREAM, 0);

	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family      = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr.sin_port        = htons(SERV_PORT);

    Bind(listenfd, (SA *) &servaddr, sizeof(servaddr));
    Listen(listenfd, LISTENQ);

	maxfd = listenfd;			/* initialize */
	maxi = -1;					/* index into client[] array */
	for (i = 0; i < FD_SETSIZE; i++)
		client[i] = -1;			/* -1 indicates available entry */
	FD_ZERO(&allset);
	FD_SET(listenfd, &allset);
/* end fig01 */

/* include fig02 */
	for ( ; ; ) {
		rset = allset;		/* structure assignment */
		nready = select(maxfd+1, &rset, NULL, NULL, NULL);


        // PART1 - new client connection
		if (FD_ISSET(listenfd, &rset)) {	/* new client connection */
			clilen = sizeof(cliaddr);
            int connfd = Accept(listenfd, (SA *) &cliaddr, &clilen);
#ifdef	NOTDEF
			printf("new client: %s, port %d\n",
					Inet_ntop(AF_INET, &cliaddr.sin_addr, 4, NULL),
					ntohs(cliaddr.sin_port));
#endif
            // send message to the new client
			for (i = 0; i < FD_SETSIZE; i++){
				if (client[i] < 0) {
					client[i] = connfd;	/* save descriptor */

					// ** 1 ** Welcome msg
                    string str_time = currentDateTime();
					string welcome_msg = str_time + " *** Welcome to the simple CHAT server\n";
                    Write(connfd, welcome_msg.c_str(), welcome_msg.length());
					
                    // ** 1 ** Name msg
					// srand( time(NULL) ); /* 設定亂數種子 */
					int x = rand()%889; /* 產生亂數 */
					string name_msg = str_time + " *** Total " + std::to_string(client_map.size()+1) + " users online now. Your name is <" + animals[x] + ">\n";
                    Write(connfd, name_msg.c_str(), name_msg.length());
					client_info tmp = client_info(animals[x], connfd, cliaddr);
					client_map.insert(pair<int, client_info>(i, tmp));

                    // broadcast user arrival message
                    string broadcast_msg = currentDateTime() +  " *** User <" + client_map[i].name + "> has just landed on the server\n";
                    broadcast(broadcast_msg, client, maxi, i, rset, allset, maxfd);

                    // server show client connect message
                    cout << "* client connected from " << inet_ntoa(cliaddr.sin_addr) << ":" << ntohs(cliaddr.sin_port) << endl;
					break;
				}
            }

			if (i == FD_SETSIZE){
				cerr << "too many clients" << endl;
				exit(0);
			} // may equal to -> err_quit("too many clients");

			FD_SET(connfd, &allset);	/* add new descriptor to set */
			if (connfd > maxfd)
				maxfd = connfd;			/* for select */
			if (i > maxi)
				maxi = i;				/* max index in client[] array */

			if (--nready <= 0)
				continue;				/* no more readable descriptors */ // -> 處理好一個人了，如果只有他那不用跑下面的 for
		}

        // PART2 - handle old connection -> need disconnect / search info
        string broadcast_msg = "";
		for (i = 0; i <= maxi; i++) {	/* check all clients for data */
			if ( (sockfd = client[i]) < 0)
				continue;
			if (FD_ISSET(sockfd, &rset)) { // check descriptor is ready -> leave, (broken), connect to host
				int n;
				memset(buf, 0, sizeof(buf));
				n = read(sockfd, buf, MAXLINE);
				// cerr << "read ready" << endl;
				if (n == 0) {  // someone leave -> collect the broadcast message and send to all user still online after the for loop
					/*4connection closed by client */
					close(sockfd);
					FD_CLR(sockfd, &allset);
					client[i] = -1;

                    // // ** **
                    broadcast_msg += currentDateTime() + " *** User <" + client_map[i].name + "> has left the server\n";
                    
                    // // server show client disconnect message
                    cout << "* client " << inet_ntoa(client_map[i].connect_info.sin_addr) << ":" << ntohs(client_map[i].connect_info.sin_port) << " disconnected" << endl;

                    // // kick the user out of the map
                    client_map.erase(i); // erase by key
                    
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

					string str_user_input = buf;

                    // user input compare
                    if( str_user_input.substr(0, 6) == "/name "){
                        if( str_user_input.length() == 7){
                            string warning = currentDateTime() + " *** No nickname given\n";
                            Write(client[i], warning.c_str(), warning.length());
                            continue;
                        }
                        // ** 2 ** /name
                        string user_want_name = str_user_input.substr(6, str_user_input.length()-7);
                        string change_name_msg = currentDateTime() + " *** Nickname changed to <" + user_want_name + ">\n";
                        Write(client[i], change_name_msg.c_str(), change_name_msg.length());

                        string broadcast_msg = currentDateTime() + " *** User <" + client_map[i].name + "> renamed to <" + user_want_name + ">\n";
                        broadcast(broadcast_msg, client, maxi, i, rset, allset, maxfd);
                        client_map[i].name = user_want_name.substr(0, user_want_name.length());

                    } else if ( str_user_input.substr(0, 4) == "/who" && str_user_input.length() == 5){
                        // ** 3 ** /who
                        Write(client[i], split_line.c_str(), split_line.length());
                        for(auto x : client_map){
                            string each_user_info = "";
                            if(x.second.connfd == client[i]) each_user_info += "* ";
                            else each_user_info += "  ";

                            each_user_info += concatenate_name(x.second.name, 21);
                            each_user_info += inet_ntoa(x.second.connect_info.sin_addr);
                            each_user_info += ":" + std::to_string(ntohs(x.second.connect_info.sin_port)) + "\n";
                            Write(client[i], each_user_info.c_str(), each_user_info.length());
                        }
                        Write(client[i], split_line.c_str(), split_line.length());
                   
                    } else if ( str_user_input.substr(0, 1) == "/" ) {
                        // ** 4 ** Unknown or incomplete command
                        string cmd_warning = currentDateTime() + " *** Unknown or incomplete command <" + str_user_input.substr(0, str_user_input.length()-1) + ">\n";
                        Write(client[i], cmd_warning.c_str(), cmd_warning.length());
                    } else {
                        // ** 5 ** regular text message
                        string broadcast_msg = currentDateTime() + "<" + client_map[i].name + "> " + str_user_input; // user input 內建換行
                        broadcast(broadcast_msg, client, maxi, i, rset, allset, maxfd);
                    }
				}

				if (--nready <= 0)
					break;				/* no more readable descriptors */
			}
		}
        broadcast(broadcast_msg, client, maxi, -1, rset, allset, maxfd); // skip set to -1 since no one need to be skip
        sleep(0.01);

	}
}
/* end fig02 */
