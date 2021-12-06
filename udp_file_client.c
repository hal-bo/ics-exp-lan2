/* -*- mode: c; coding:utf-8; ##: nil; -*-                          */
/*                                                                  */
/*  FILENAME     :  lab_udp_ecli.c                                  */
/*  DESCRIPTION  :  UDP echo clientの練習                           */
/*                                                                  */
/*  DATE         :  Sep. 01, 2020                                   */
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

int main(int argc, char **argv)
{
    struct epoll_event events[MAX_EVENTS]; /* イベントとイベントの配列 */
    int epfd;       /* epollインスタンスのファイルデスクリプタ */
    int nfds;       /* 要求するI/Oが可能なｆｄの数 */
    int i;
    int cnt = 0;
    char *filename = NULL;
    char *dummy_file = "HELLO.txt";               /* ダミーのリクエストメッセージ */
    int fd = 1;                             /* 標準出力 */

    int sock;                       /* ソケットディスクリプタ */
    struct sockaddr_in serverAddr;  /* サーバ＝相手用のアドレス構造体 */
    struct sockaddr_in clientAddr;  /* クライアント＝相手用アドレス構造体 */
    int addrLen;                    /* serverAddrのサイズ */
    char buf[BUF_LEN];              /* 受信バッファ */
    int n;                          /* 読み込み／受信バイト数 */

    struct in_addr addr;            /* アドレス表示用 */

    char *server_name_str = SERVER_HOSTNAME;
    char *port_num_str = UDP_SERVER_PORT_STR;
    struct addrinfo hints, *res;
    int err;

    unsigned int sec;
    int nsec;
    double time;
    struct timespec start_time, end_time;

    /* コマンドライン引数の処理 */
    if(argc == 2 && strncmp(argv[1], "-h", 2) == 0) {
        printf("Usage: %s [dst_host] [port]\n", argv[0]);
        return 0;
    }
    if (argc == 2) {
        server_name_str = argv[1];
    } else if (argc >= 3) {
        server_name_str = argv[1];
        port_num_str = argv[2];
    }
    if(argc > 3) {  /* 出力ファイルの指定 */
      printf("set outputfile: %s\n", argv[3]);
      filename = argv[3];
      /* STEP 0: 出力ファイルのオープン */
      fd = open(filename, O_CREAT | O_WRONLY, 0644);
      if(fd < 0) {
          perror("open");
          return 1;
      }
    }
    /* STEP 1: 宛先サーバのIPアドレスとポートを指定する */
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;

    // ホスト名からIPアドレスを解決する
    err = getaddrinfo(server_name_str, port_num_str, &hints, &res);
    if (err != 0) {
        perror("getaddrinfo");
        return 1;
    }

    /* 確認用：IPアドレスとポートを文字列に変換して表示 */
    addr.s_addr = ((struct sockaddr_in *)(res->ai_addr))->sin_addr.s_addr;
    printf("ip address: %s\n", inet_ntoa(addr));
    printf("port#: %d\n", ntohs(((struct sockaddr_in*)(res->ai_addr))->sin_port));

    /* STEP 2xxx: UDPソケットをオープンする */
    sock = socket(res->ai_family, res->ai_socktype, 0);
    if(sock < 0) {
        perror("socket");
        return  1;
    }

    memset(&serverAddr, 0, sizeof(serverAddr));     /* ゼロクリア */
    serverAddr.sin_family = AF_INET;                /* Internetプロトコル */
    serverAddr.sin_port = htons(UDP_SERVER_PORT); /* ADD HERE */
    serverAddr.sin_addr.s_addr = htonl(INADDR_ANY); /* どのIPアドレス宛でも */

    /* STEP 3:ソケットとアドレスをbindする */
    if(bind(sock, (struct sockaddr *)&serverAddr, sizeof(serverAddr) /* ADD HERE */ ) < 0) {
        perror("bind");
        return  1;
    }

    epfd = epoll_create(MAX_EVENTS);
    if (epfd < 0) {
        perror("epoll_create");
        return 1;
    }
    if(epoll_ctl_add_in(epfd, sock) != 0) {
        perror("epoll_ctrl_add_in");
        return 1;
    }

    sprintf(buf, "GET %s\r\n", dummy_file);
    if(strlen(buf) != sendto(sock, buf, strlen(buf), 0, res->ai_addr, res->ai_addrlen)) {
        perror("sendto");
        return  -1;
    }
    for( ; ; ) {

        /* STEP 3: イベント発生をを待つ */
        /* nfdsには処理が可能になったイベント数が返される */
        nfds = epoll_wait(epfd, events, MAX_EVENTS, 30000);
        if(nfds < 0) {
            perror("epoll_wait");
            return 1;
        }

        /* タイムアウト */
        if(nfds == 0) {
            printf("timeout %d\n", cnt++);
            continue;
        }

        /* 処理可能なデスクリプタが events[] 登録されている */
        /* STEP 4: 順次確認して必要な処理を行う */
        for(i = 0; i < nfds; i++) {
            if (events[i].data.fd == sock) {
                addrLen = sizeof(clientAddr);
                n = recvfrom(sock, buf, BUF_LEN, 0,
                    (struct sockaddr *)&clientAddr, (socklen_t *)&addrLen);/* ADD HERE */;
                //clock_gettime(CLOCK_REALTIME, &end_time);
                addr.s_addr = clientAddr.sin_addr.s_addr;
                //printf("received from : ip Address: %s ", inet_ntoa(addr));
                //printf("port#: %d\n", ntohs(clientAddr.sin_port));
                if(n < 0) {
                    perror("recvfrom");
                }

                /* 受信したデータとそのバイト数の表示 */
                //buf[n] = '\0';
                if (strcmp(buf, "quit") == 0) {
                  printf("finish!\n");
                } else {
                  //printf("%s: %s\n",server_name_str,buf);
                  write(fd, buf, n);  /* ファイルに出力 */
                  printf("*");
                  //printf("Recv %d bytes data: %s", n, buf);
                  //printf("\n");
                }
            }
        }
    }

    /* STEP 5: ソケットのクローズ */
    close(sock);

    return 0;

}

