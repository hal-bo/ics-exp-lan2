/* -*- mode: c; coding:utf-8; ##: nil; -*-                          */
/*                                                                  */
/*  FILENAME     :  lab_udp_esrv.c                                  */
/*  DESCRIPTION  :  UDP echo serverの練習                           */
/*                                                                  */
/*  DATE         :  Sep. 01, 2020                                   */
/*                                                                  */

#include "icslab2_net.h"

int
main(int argc, char** argv)
{
    int     sock;                  /* ソケットディスクリプタ */
    struct sockaddr_in  serverAddr; /* サーバ＝自分用アドレス構造体 */
    struct sockaddr_in  clientAddr; /* クライアント＝相手用アドレス構造体 */
    int     addrLen;                /* clientAddrのサイズ */
    char    buf[BUF_LEN];          /* 受信バッファ */
    int     n;                      /* 受信バイト数 */

    char *filename;                 /* 返送するファイルの名前 */
    int fd;                         /* ファイルデスクリプタ */

    struct in_addr addr;            /* アドレス表示用 */

    char *server_name_str = SERVER_HOSTNAME;
    char *port_num_str = UDP_SERVER_PORT_STR;
    struct addrinfo hints, *res;
    int err;


    /* コマンドライン引数の処理 */
    if(argc == 2 && strncmp(argv[1], "-h", 2) == 0) {
        printf("Usage: %s\n", argv[0]);
        printf("       default port # %d\n", UDP_SERVER_PORT);
        return 0;
    }

    if (argc == 2) {
        server_name_str = argv[1];
    } else if (argc >= 3) {
        server_name_str = argv[1];
        port_num_str = argv[2];
    }

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;

    // ホスト名からIPアドレスを解決する
    err = getaddrinfo(server_name_str, port_num_str, &hints, &res);
    if (err != 0) {
        perror("getaddrinfo");
        return 1;
    }

    addr.s_addr = ((struct sockaddr_in *)(res->ai_addr))->sin_addr.s_addr;
    printf("ip address: %s\n", inet_ntoa(addr));
    printf("port#: %d\n", ntohs(((struct sockaddr_in*)(res->ai_addr))->sin_port));

    /* STEP 1: UDPソケットをオープンする */
    sock = socket(res->ai_family, res->ai_socktype, 0);
    //sock = socket(AF_INET, SOCK_DGRAM, 0);
    if(sock < 0) {
        perror("socket");
        return  1;
    }

    /* STEP 2: クライアントからの要求を受け付けるIPアドレスとポートを設定する */
    memset(&serverAddr, 0, sizeof(serverAddr));     /* ゼロクリア */
    serverAddr.sin_family = AF_INET;                /* Internetプロトコル */

    /* STEP 2 xxx: 待ち受けるポート番号を 10000 (= UDP_SERVER_PORT)に設定 */
    serverAddr.sin_port = htons(UDP_SERVER_PORT); /* ADD HERE */

    serverAddr.sin_addr.s_addr = htonl(INADDR_ANY); /* どのIPアドレス宛でも */

    /* STEP 3:ソケットとアドレスをbindする */
    if(bind(sock, (struct sockaddr *)&serverAddr, sizeof(serverAddr) /* ADD HERE */ ) < 0) {
        perror("bind");
        return  1;
    }

    for( ; ; ) {                            /* 無限ループ */
        /* STEP 4: クライアントからのデータグラムを受けとる */
        //addrLen = sizeof(clientAddr);


        /* （後回し） STEP 4'xxx. 受信パケットの送信元IPアドレスとポート番号を表示 */
        /* ADD HERE */
        // addr.s_addr = clientAddr.sin_addr.s_addr;
        // printf("received from : ip Address: %s ", inet_ntoa(addr));
        // printf("port#: %d\n", ntohs(clientAddr.sin_port));
        n = recvfrom(sock, buf, BUF_LEN, 0,
                     (struct sockaddr *)&clientAddr, (socklen_t *)&addrLen);
        if(n < 0) {
            perror("recvfrom");
            break;
        }
        //uf[n] = '\0';
        //printf("receive: %s\n", buf);
        printf("*");
        if(sendto(sock, buf, n, 0,res->ai_addr, res->ai_addrlen) /* ADD HERE */ != n) {
            perror("sendto");
            break;
        }

        //printf("\n");
    }

    close(sock);                               /* ソケットのクローズ */

    return  0;
}

/* Option 1 コマンドラインでの、待ち受けポートの指定　*/
/* コマンドライン引数で指定がなければ UDP_SERVER_PORT (=10000)に */
/* 指定があれば、そのポートに設定                               */
/* Hint: 型に注意   atoi(), atof() */

