#include <arpa/inet.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>

#define BOMB -1
#define HIDDEN -2
#define FLAG -3

typedef struct action {
  int type;
  int coordinates[2];
  int board[4][4];
} Action;

typedef struct campoMinado {
  int resposta[4][4];
  int estado[4][4];
} CampoMinado;

void logexit(const char *msg);

int addrparse(const char *addrstr, const char *portstr,
              struct sockaddr_storage *storage);

void addrtostr(const struct sockaddr *addr, char *str, size_t strsize);

int server_sockaddr_init(const char *proto, const char *portstr,
                         struct sockaddr_storage *storage);

void start(CampoMinado *campoMinado, char *nomeArquivo);

void reveal(int *coordinates, CampoMinado *campoMinado);

void flag(int *coordinates, CampoMinado *campoMinado);

void remove_flag(int *coordinates, CampoMinado *campoMinado);

void reset(CampoMinado *campoMinado);

void setCoordinates(int *coordinates);

Action setActionClient();

void printEstado(CampoMinado *campoMinado);