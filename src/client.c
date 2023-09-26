#include <arpa/inet.h>
#include <common.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

int main(int argc, char** argv) {
    // criacao do socket e conexao com servidor
    struct sockaddr_storage storage;
    if(addrparse(argv[1], argv[2], &storage) != 0) logexit("addr");

    int s = socket(storage.ss_family, SOCK_STREAM, 0);
    if(s == -1) logexit("socket");

    struct sockaddr* addr = (struct sockaddr*)(&storage);
    if(connect(s, addr, sizeof(storage)) != 0) logexit("connect");

    char addrstr[BUFSZ];
    addrtostr(addr, addrstr, BUFSZ);

    int board[4][4];
    for(int i = 0; i < 4; i++) {
        for(int j = 0; j < 4; j++) {
            board[i][j] = HIDDEN;
        }
    }

    int started = 0;

    while(1) {
        // envio de mensagens
        char buf[sizeof(Action)];
        memset(buf, 0, sizeof(Action));
        Action action;
        do {
            action = setActionClient(board);
            if(action.type == 0) {
                started = 1;
            }
        } while(action.type == -1 || started == 0);
        action = endianessSend(action);
        memcpy(buf, &action, sizeof(Action));

        size_t count = send(s, buf, sizeof(Action), 0);

        if(count != sizeof(Action)) {
            logexit("send");
        }

        // recebimento de mensagens
        memset(buf, 0, sizeof(Action));
        count = recv(s, buf, sizeof(Action), 0);
        memcpy(&action, buf, sizeof(Action));
        action = endianessRcv(action);

        // tratamento
        if(action.type == 3) {
            for(int i = 0; i < 4; i++) {
                for(int j = 0; j < 4; j++) {
                    board[i][j] = action.board[i][j];
                }
            }
            printBoard(action.board);
        }
        else if(action.type == 6) {
            printf("YOU WIN!\n");
            printBoard(action.board);
            break;
        }
        else if(action.type == 8) {
            printf("GAME OVER!\n");
            printBoard(action.board);
            break;
        }
        else {
            break;
        }
    }

    close(s);

    return 0;
}