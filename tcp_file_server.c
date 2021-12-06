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
    char *server_name_str[4];
    char *port_num_str = UDP_SERVER_PORT_STR;
    struct addrinfo hints, *res;
    int err;

    struct sockaddr_in clientAddr;  /* クライアント＝相手用アドレス構造体 */
    int     addrLen;                /* clientAddrのサイズ */

    int clientSock[4];
    int serverSock;
    int sock;

    char    buf[BUF_LEN];           /* 受信バッファ */
    int     n;                      /* 受信バイト数 */
    int     isEnd = 0;              /* 終了フラグ，0でなければ終了 */

    char *file;
    char *filename;                 /* 返送するファイルの名前 */
    int fd[4];                         /* ファイルデスクリプタ */
    int i;

    int cnt = 0;

    int     yes = 1;                /* setsockopt()用 */

    unsigned int sec;
    int nsec;
    double time;
    struct timespec start_time, end_time;

    /* コマンドライン引数の処理 */
    // if(argc != 2) {
    //     printf("Usage: %s filename\n", argv[0]);
    //     printf("ex. %s http_get_req.txt\n", argv[0]);
    //     return 0;
    // }

    filename = argv[1];
    for(i=0;i<4;i++){
        sprintf(file, "%d%s", i, filename);
        printf("set outputfile: %s\n", file);
        fd[i] = open(file, O_RDONLY);
        if(fd[i] < 0) {
            perror("open");
            return 1;
        }
    }
    for (i = 2; i < argc; i++) {
        server_name_str[i-2] = argv[i];
        clientSock[i-2] = createClientSocket(server_name_str[i-2], port_num_str);
    }
    
    serverSock = createServerSocket();

    while(!isEnd) {     /* 終了フラグが0の間は繰り返す */
        addrLen = sizeof(clientAddr);
        sock = accept(serverSock, (struct sockaddr *)&clientAddr, (socklen_t *)&addrLen);
        if(sock < 0) {
            perror("accept");
            return  1;
        }
        /* STEP 6: クライアントからのファイル要求の受信 */
        if((n = read(sock, buf, BUF_LEN)) < 0) {
            close(sock);
            break;
        }

        /* 今回は表示するだけで中身は無視 */
        buf[n] = '\0';
        printf("recv req: %s\n", buf);
        clock_gettime(CLOCK_REALTIME, &start_time);
        for (i = 2; i < argc; i++) {
            while((n = read(fd[i-2], buf, BUF_LEN)) > 0) {
                write(clientSock[i-2], buf, n);
            }
        }

        clock_gettime(CLOCK_REALTIME, &end_time);
        sec = end_time.tv_sec - start_time.tv_sec;
        nsec = end_time.tv_nsec - start_time.tv_nsec;
        time = (double)sec + (double)nsec * 1e-6;
        printf("RCC: %lf ms\n", time);
        printf("send count: %d\n", cnt);
    }

    for (i = 2; i < argc; i++) {
        close(fd[i-2]);
        close(clientSock[i-2]);
    }

    return  0;
}

/* Local Variables: */
/* compile-command: "gcc tcp_file_server.c -o tcp_file_server.out" */
/* End: */
