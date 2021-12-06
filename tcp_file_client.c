/*  -*- coding: utf-8-unix; -*-                                     */
/*  FILENAME     :  tcp_file_client.c                               */
/*  DESCRIPTION  :  TCP file Client                                 */
/*                                                                  */
/*  VERSION      :                                                  */
/*  DATE         :  Sep 01, 2020                                    */
/*  UPDATE       :                                                  */
/*                                                                  */

#include "icslab2_net.h"
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/epoll.h>

#define MAX_EVENTS 10
#define MAX_LEN 256

int epoll_ctl_add_in(int epfd, int fd);

int epoll_ctl_add_in(int epfd, int fd)
{
    struct epoll_event ev; /* イベント */

    memset(&ev, 0, sizeof(ev)); /* 0クリア */
    ev.events = EPOLLIN;        /* read()可能というイベント */
    ev.data.fd = fd;            /* 関連付けるfd */
    if(epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &ev) < 0) {
        perror("epoll_ctl");
        return 1;
    }
    return 0;
}

int createServerSocket() {
    int     sock;                  /* 待ち受け用ソケットディスクリプタ */
    struct sockaddr_in  serverAddr; /* サーバ＝自分用アドレス構造体 */
    int     yes = 1;                /* setsockopt()用 */
    if((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socke");
        return  1;
    }
    /* sock0のコネクションがTIME_WAIT状態でもbind()できるように設定 */
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR,
               (const char *)&yes, sizeof(yes));
    memset(&serverAddr, 0, sizeof(serverAddr));     /* ゼロクリア */
    serverAddr.sin_family = AF_INET;                /* Internetプロトコル */
    serverAddr.sin_port = htons(TCP_SERVER_PORT);   /* 待ち受けるポート */
    serverAddr.sin_addr.s_addr = htonl(INADDR_ANY); /* どのIPアドレス宛でも */

    if(bind(sock, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0) {
        perror("bind");
        return  1;
    }

    if(listen(sock, 5) != 0) {
        perror("listen");
        return  1;
    }
    printf("waiting connection...\n");
    return sock;
}
int createClientSocket(char *server_name_str, char *port_num_str) {
    struct sockaddr_in serverAddr;  /* サーバ＝相手用のアドレス構造体 */
    int     sock;                  /* ソケットディスクリプタ */
    struct addrinfo hints, *res;
    int err;
    struct in_addr addr;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    err = getaddrinfo(server_name_str, port_num_str, &hints, &res);
    if (err != 0) {
        perror("getaddrinfo");
        return 1;
    }
    addr.s_addr = ((struct sockaddr_in *)(res->ai_addr))->sin_addr.s_addr;
    printf("ip address: %s\n", inet_ntoa(addr));
    printf("port#: %d\n", ntohs(((struct sockaddr_in*)(res->ai_addr))->sin_port));

    sock = socket(res->ai_family, res->ai_socktype, 0);
    if(sock < 0) {
        perror("socket");
        return  1;
    }
    if(connect(sock, res->ai_addr, res->ai_addrlen) < 0) {
        perror("connect");
        return  1;
    }
    return sock;
}
int
main(int argc, char** argv)
{
    struct epoll_event events[MAX_EVENTS]; /* イベントとイベントの配列 */
    int epfd;       /* epollインスタンスのファイルデスクリプタ */
    int nfds;       /* 要求するI/Oが可能なｆｄの数 */
    int i;
    char *file = NULL;
    char *filename = NULL;
    char *dummy_file = "HELLO.txt";               /* ダミーのリクエストメッセージ */
    int fd[4];                             /* 標準出力 */
  
    int clientSock[4];
    char buf[BUF_LEN];              /* 受信バッファ */
    int n;                          /* 読み込み／受信バイト数 */

    char *server_name_str[4];
    char *port_num_str = UDP_SERVER_PORT_STR;
    struct addrinfo hints, *res;
    int err;

    struct in_addr addr;            /* アドレス表示用 */

    int fcnt = 0;

    epfd = epoll_create(MAX_EVENTS);
    if (epfd < 0) {
        perror("epoll_create");
        return 1;
    }
    /* コマンドライン引数の処理 */
    if(argc == 2 && strncmp(argv[1], "-h", 2) == 0) {
        printf("Usage: %s [dst_ip_addr] [port] [o_filename]\n", argv[0]);
        return 0;
    }
    printf("set outputfile: %s\n", argv[1]);
    filename = argv[1];
    for(i=2;i<argc;i++){
        sprintf(file, "%d%s", i-2, filename);
        fd[i-2] = open(filename, O_CREAT | O_WRONLY, 0644);
        if(fd[i-2] < 0) {
            perror("open");
            return 1;
        }
    }
    for (i = 2; i < argc; i++) {
        server_name_str[i-2] = argv[i];
        clientSock[i-2] = createClientSocket(server_name_str[i-2], port_num_str);
        if(epoll_ctl_add_in(epfd, clientSock[i-2]) != 0) {
            perror("epoll_ctrl_add_in");
            return 1;
        }
    }

    sprintf(buf, "GET %s\r\n", dummy_file);
    if(strlen(buf) != write(clientSock[0], buf, strlen(buf))) {
        perror("request send\n");
        return  -1;
    }

    while(fcnt < argc - 2) {

        nfds = epoll_wait(epfd, events, MAX_EVENTS, 30000);
        if(nfds < 0) {
            perror("epoll_wait");
            return 1;
        }
        if(nfds == 0) {
            printf("timeout\n");
            continue;
        }
        for(i = 0; i < nfds; i++) {
            for (i = 2; i < argc; i++) {
                if (events[i].data.fd == clientSock[i-2]) {
                    n = read(clientSock[i-2], buf, BUF_LEN);/* ADD HERE */;
                    if(n < 0) {
                        perror("recvfrom");
                    }
                    if (strcmp(buf, "quit") == 0) {
                        printf("finish! [%d]\n", i-2);
                        fcnt++;
                    } else {
                        write(fd[i-2], buf, n);  /* ファイルに出力 */
                        printf("%d", i-2);
                    }
                }
            }
        }
    }

    for (i = 2; i < argc; i++) {
        close(fd[i-2]);
        close(clientSock[i-2]);
    }
    return 0;
}
/*--------------------------- <end> --------------------------------*/
