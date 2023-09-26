#include <common.h>

void logexit(const char* msg) {
    perror(msg);
    exit(1);
}

int addrparse(const char* addrstr, const char* portstr, struct sockaddr_storage* storage) {
    if(addrstr == NULL || portstr == NULL) {
        return -1;
    }

    __uint16_t port = (__uint16_t)atoi(portstr);
    if(port == 0) {
        return -1;
    }
    port = htons(port);

    struct in_addr inaddr4;
    if(inet_pton(AF_INET, addrstr, &inaddr4)) {
        struct sockaddr_in* addr4 = (struct sockaddr_in*)storage;
        addr4->sin_family = AF_INET;
        addr4->sin_port = port;
        addr4->sin_addr = inaddr4;
        return 0;
    }

    struct in6_addr inaddr6;
    if(inet_pton(AF_INET6, addrstr, &inaddr6)) {
        struct sockaddr_in6* addr6 = (struct sockaddr_in6*)storage;
        addr6->sin6_family = AF_INET6;
        addr6->sin6_port = port;
        memcpy(&(addr6->sin6_addr), &inaddr6, sizeof(inaddr6));
        return 0;
    }

    return -1;
}

void addrtostr(const struct sockaddr* addr, char* str, size_t strsize) {
    int version;
    char addrstr[INET6_ADDRSTRLEN + 1] = "";
    uint16_t port;
    if(addr->sa_family == AF_INET) {
        version = 4;
        struct sockaddr_in* addr4 = (struct sockaddr_in*)addr;
        if(!inet_ntop(AF_INET, &(addr4->sin_addr), addrstr, INET6_ADDRSTRLEN + 1)) {
            logexit("ntop");
        }
        port = ntohs(addr4->sin_port);
    }
    else if(addr->sa_family == AF_INET6) {
        version = 6;
        struct sockaddr_in6* addr6 = (struct sockaddr_in6*)addr;
        if(!inet_ntop(AF_INET6, &(addr6->sin6_addr), addrstr, INET6_ADDRSTRLEN + 1)) {
            logexit("ntop");
        }
        port = ntohs(addr6->sin6_port);
    }
    else {
        logexit("unknown protocol family");
    }
    if(str) {
        snprintf(str, strsize, "IPv%d %s %hu", version, addrstr, port);
    }
}

int server_sockaddr_init(const char* proto, const char* portstr, struct sockaddr_storage* storage) {
    __uint16_t port = (__uint16_t)atoi(portstr);
    if(port == 0) {
        return -1;
    }
    port = htons(port);

    memset(storage, 0, sizeof(*storage));
    if(strcmp(proto, "v4") == 0) {
        struct sockaddr_in* addr4 = (struct sockaddr_in*)storage;
        addr4->sin_family = AF_INET;
        addr4->sin_port = port;
        addr4->sin_addr.s_addr = INADDR_ANY;
        return 0;
    }
    else if(strcmp(proto, "v6") == 0) {
        struct sockaddr_in6* addr6 = (struct sockaddr_in6*)storage;
        addr6->sin6_family = AF_INET6;
        addr6->sin6_port = port;
        addr6->sin6_addr = in6addr_any;
        return 0;
    }
    else {
        return -1;
    }
}

Action endianessSend(Action action) {
    action.type = htonl(action.type);
    action.coordinates[0] = htonl(action.coordinates[0]);
    action.coordinates[1] = htonl(action.coordinates[1]);

    for(int i = 0; i < 4; i++) {
        for(int j = 0; j < 4; j++) {
            action.board[i][j] = htonl(action.board[i][j]);
        }
    }

    return action;
}

Action endianessRcv(Action action) {
    action.type = ntohl(action.type);
    action.coordinates[0] = ntohl(action.coordinates[0]);
    action.coordinates[1] = ntohl(action.coordinates[1]);

    for(int i = 0; i < 4; i++) {
        for(int j = 0; j < 4; j++) {
            action.board[i][j] = ntohl(action.board[i][j]);
        }
    }
    return action;
}

void state(CampoMinado* campoMinado, Action* action) {
    for(int i = 0; i < 4; i++) {
        for(int j = 0; j < 4; j++) {
            action->board[i][j] = campoMinado->estado[i][j];
        }
    }
    action->type = 3;
    action->coordinates[0] = -1;
    action->coordinates[1] = -1;
}

Action setActionServer(CampoMinado* campoMinado, Action action, char* nomeArquivo) {
    Action newAction;
    // start
    if(action.type == 0) {
        start(campoMinado, nomeArquivo);
        state(campoMinado, &newAction);
    }
    // reveal
    else if(action.type == 1) {
        int acao = reveal(action.coordinates, campoMinado);
        if(acao == 0) {
            state(campoMinado, &newAction);
        }
        else if(acao == 1) {
            for(int i = 0; i < 4; i++) {
                for(int j = 0; j < 4; j++) {
                    newAction.board[i][j] = campoMinado->resposta[i][j];
                }
            }
            newAction.type = 6;
            newAction.coordinates[0] = -1;
            newAction.coordinates[1] = -1;
        }
        else {
            for(int i = 0; i < 4; i++) {
                for(int j = 0; j < 4; j++) {
                    newAction.board[i][j] = campoMinado->resposta[i][j];
                }
            }
            newAction.type = 8;
            newAction.coordinates[0] = -1;
            newAction.coordinates[1] = -1;
        }
    }
    // flag
    else if(action.type == 2) {
        flag(action.coordinates, campoMinado);
        state(campoMinado, &newAction);
    }
    // remove_flag
    else if(action.type == 4) {
        remove_flag(action.coordinates, campoMinado);
        state(campoMinado, &newAction);
    }
    // reset
    else if(action.type == 5) {
        reset(campoMinado);
        state(campoMinado, &newAction);
        printf("starting new game\n");
    }
    // exit
    else if(action.type == 7) {
        reset(campoMinado);
        printf("client disconnected\n");
        newAction.type = -2;
    }
    else {
        logexit("tratamento");
    }
    return newAction;
}

void start(CampoMinado* campoMinado, char* nomeArquivo) {
    FILE* arquivo;
    char linha[20];
    char* token;
    int linhaAtual = 0;

    arquivo = fopen(nomeArquivo, "r");

    if(arquivo == NULL) {
        perror("Erro ao abrir o arquivo");
        exit(1);
    }

    while(fgets(linha, sizeof(linha), arquivo) && linhaAtual < 4) {
        token = strtok(linha, ",");
        int colunaAtual = 0;

        while(token != NULL && colunaAtual < 4) {
            campoMinado->resposta[linhaAtual][colunaAtual] = atoi(token);
            campoMinado->estado[linhaAtual][colunaAtual] = HIDDEN;
            token = strtok(NULL, ",");
            colunaAtual++;
        }

        linhaAtual++;
    }

    fclose(arquivo);
}

int reveal(int* coordinates, CampoMinado* campoMinado) {
    if(campoMinado->resposta[coordinates[0]][coordinates[1]] == BOMB) {
        return -1;
    }
    campoMinado->estado[coordinates[0]][coordinates[1]] = campoMinado->resposta[coordinates[0]][coordinates[1]];

    int num_bombas = 0;
    int num_hidden_or_flag = 0;
    for(int i = 0; i < 4; i++) {
        for(int j = 0; j < 4; j++) {
            if(campoMinado->resposta[i][j] == BOMB) {
                num_bombas++;
            }
            if(campoMinado->estado[i][j] == HIDDEN || campoMinado->estado[i][j] == FLAG) {
                num_hidden_or_flag++;
            }
        }
    }
    if(num_bombas == num_hidden_or_flag) {
        return 1;
    }

    return 0;
}

void flag(int* coordinates, CampoMinado* campoMinado) { campoMinado->estado[coordinates[0]][coordinates[1]] = FLAG; }

void remove_flag(int* coordinates, CampoMinado* campoMinado) {
    if(campoMinado->estado[coordinates[0]][coordinates[1]] == FLAG) {
        campoMinado->estado[coordinates[0]][coordinates[1]] = HIDDEN;
    }
}

void reset(CampoMinado* campoMinado) {
    for(int i = 0; i < 4; i++) {
        for(int j = 0; j < 4; j++) {
            campoMinado->estado[i][j] = HIDDEN;
        }
    }
}

void setCoordinates(int* coordinates) {
    char input[10];
    scanf("%s", input);
    sscanf(input, "%d,%d", &coordinates[0], &coordinates[1]);
}

Action setActionClient(int board[4][4]) {
    Action act;
    char type[20];
    int coordinates[2] = {-1, -1};

    scanf("%s", type);

    if(strcmp(type, "start") == 0) {
        act.type = 0;
    }
    else if(strcmp(type, "reveal") == 0) {
        act.type = 1;
        setCoordinates(coordinates);
        if(coordinates[0] > 3 || coordinates[0] < 0 || coordinates[1] > 3 || coordinates[1] < 0) {
            printf("error: invalid cell\n");
            act.type = -1;
        }
        else if(board[coordinates[0]][coordinates[1]] != HIDDEN && board[coordinates[0]][coordinates[1]] != FLAG) {
            printf("error: cell already revealed\n");
            act.type = -1;
        }
    }
    else if(strcmp(type, "flag") == 0) {
        act.type = 2;
        setCoordinates(coordinates);
        if(board[coordinates[0]][coordinates[1]] == FLAG) {
            printf("error: cell already has a flag\n");
            act.type = -1;
        }
        else if(board[coordinates[0]][coordinates[1]] != HIDDEN) {
            printf("error: cannot insert flag in revealed cell\n");
            act.type = -1;
        }
    }
    else if(strcmp(type, "remove_flag") == 0) {
        act.type = 4;
        setCoordinates(coordinates);
    }
    else if(strcmp(type, "reset") == 0) {
        act.type = 5;
    }
    else if(strcmp(type, "exit") == 0) {
        act.type = 7;
    }
    else if(strcmp(type, "game_over") == 0) {
        act.type = 8;
    }
    else {
        printf("error: command not found\n");
        act.type = -1;
    }
    act.coordinates[0] = coordinates[0];
    act.coordinates[1] = coordinates[1];
    return act;
}

void printBoard(int board[4][4]) {
    for(int i = 0; i < 4; i++) {
        for(int j = 0; j < 4; j++) {
            if(board[i][j] == BOMB) {
                printf("*");
            }
            else if(board[i][j] == HIDDEN) {
                printf("-");
            }
            else if(board[i][j] == FLAG) {
                printf(">");
            }
            else {
                printf("%d", board[i][j]);
            }
            if(j != 3) {
                printf("\t\t");
            }
        }
        printf("\n");
    }
}

void printAction(Action action) {
    printf("Type: %d  %d,%d\n", action.type, action.coordinates[0], action.coordinates[1]);
    printBoard(action.board);
}
