#include <arpa/inet.h>
#include <common.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

int main(int argc, char** argv) {
    CampoMinado campoMinado;
    start(&campoMinado, argv[4]);
    printBoard(campoMinado.resposta);

    // criacao do socket e conexao com cliente
    struct sockaddr_storage storage;
    if(server_sockaddr_init(argv[1], argv[2], &storage) != 0) {
        logexit("addr");
    }

    int s = socket(storage.ss_family, SOCK_STREAM, 0);
    if(s == -1) {
        logexit("socket");
    }

    int enable = 1;
    if(setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) != 0) {
        logexit("setsocket");
    }

    struct sockaddr* addr = (struct sockaddr*)(&storage);
    if(bind(s, addr, sizeof(storage)) != 0) {
        logexit("bind");
    }

    while(1) {
        if(listen(s, 1)) {
            logexit("listen");
        }

        struct sockaddr_storage cstorage;
        struct sockaddr* caddr = (struct sockaddr*)(&cstorage);
        socklen_t caddrlen = sizeof(cstorage);

        int csock = accept(s, caddr, &caddrlen);
        if(csock == -1) {
            logexit("accept");
        }

        printf("client connected\n");

        while(1) {
            // recebimento de mensagens
            char buf[sizeof(Action)];
            memset(buf, 0, sizeof(Action));
            Action action;
            size_t count = recv(csock, buf, sizeof(Action), 0);
            memcpy(&action, buf, sizeof(Action));
            action = endianessRcv(action);

            // tratamento
            Action newAction;
            newAction = setActionServer(&campoMinado, action, argv[4]);
            if(newAction.type == -2) {
                break;
            }

            // envio de mensagens
            memset(buf, 0, sizeof(Action));
            int type = newAction.type;
            newAction = endianessSend(newAction);
            memcpy(buf, &newAction, sizeof(Action));
            count = send(csock, buf, sizeof(Action), 0);
            if(count != sizeof(Action)) {
                logexit("send");
            }
            if(type == 8 || type == 6) {
                break;
            }
        }
        close(csock);
    }

    close(s);

    exit(1);

    return 0;
}
