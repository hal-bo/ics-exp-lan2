/*  -*- coding: utf-8-unix; -*-                                     */
/*  FILENAME     :  tcp_file_server.c                                 */
/*  DESCRIPTION  :  Simple TCP file Server                          */
/*  USAGE:          tcp_file_server.out filename                    */
/*  VERSION      :                                                  */
/*  DATE         :  Sep 01, 2020                                    */
/*  UPDATE       :                                                  */
/*                                                                  */

#include "icslab2_net.h"

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

int createClientSocket() {
    char *server_ipaddr_str = "172.20.0.10";  /* サーバIPアドレス（文字列） */
    unsigned int port = TCP_SERVER_PORT;    /* ポート番号 */
    struct sockaddr_in serverAddr;  /* サーバ＝相手用のアドレス構造体 */
    int     sock;                  /* ソケットディスクリプタ */
    /* STEP 1: 宛先サーバのIPアドレスとポートを指定 */
    memset(&serverAddr, 0, sizeof(serverAddr));     /* 0クリア */
    serverAddr.sin_family = AF_INET;                /* Internetプロトコル */
    serverAddr.sin_port = htons(port);              /* サーバの待受ポート */
    /* IPアドレス（文字列）から変換 */
    inet_pton(AF_INET, server_ipaddr_str, &serverAddr.sin_addr.s_addr); 
    if((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket");
        return  1;
    }
    if(connect(sock, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        perror("connect");
        return  1;
    }
    return sock;
}
int
main(int argc, char** argv)
{
    int serverSock;
    int connectedSock;
    char    buf[BUF_LEN];           /* 受信バッファ */
    int     n;                      /* 受信バイト数 */

    serverSock = createServerSocket();
    while (1) {
        struct sockaddr_in addr;
        socklen_t addrLen;
        addrLen = sizeof(addr);
        connectedSock = accept(serverSock, (struct sockaddr *)&addr, (socklen_t *)&addrLen);
        if(connectedSock < 0) {
            perror("accept");
            return  1;
        }
        int clientSock = createClientSocket();
        while((n = read(connectedSock, buf, BUF_LEN)) > 0) {
            printf("%s", buf);
            write(clientSock, buf, n);
        }
        printf("\nsend\n");
        close(clientSock);
    }
    return  0;
}

/* Local Variables: */
/* compile-command: "gcc tcp_file_server.c -o tcp_file_server.out" */
/* End: */
