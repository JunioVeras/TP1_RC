#include <common.h>

void logexit(const char *msg) {
  perror(msg);
  exit(1);
}

int addrparse(const char *addrstr, const char *portstr,
              struct sockaddr_storage *storage) {
  if (addrstr == NULL || portstr == NULL) {
    return -1;
  }

  __uint16_t port = (__uint16_t)atoi(portstr);
  if (port == 0) {
    return -1;
  }
  port = htons(port);

  struct in_addr inaddr4;
  if (inet_pton(AF_INET, addrstr, &inaddr4)) {
    struct sockaddr_in *addr4 = (struct sockaddr_in *)storage;
    addr4->sin_family = AF_INET;
    addr4->sin_port = port;
    addr4->sin_addr = inaddr4;
    return 0;
  }

  struct in6_addr inaddr6;
  if (inet_pton(AF_INET6, addrstr, &inaddr6)) {
    struct sockaddr_in6 *addr6 = (struct sockaddr_in6 *)storage;
    addr6->sin6_family = AF_INET6;
    addr6->sin6_port = port;
    memcpy(&(addr6->sin6_addr), &inaddr6, sizeof(inaddr6));
    return 0;
  }

  return -1;
}

void addrtostr(const struct sockaddr *addr, char *str, size_t strsize) {
  int version;
  char addrstr[INET6_ADDRSTRLEN + 1] = "";
  uint16_t port;
  if (addr->sa_family == AF_INET) {
    version = 4;
    struct sockaddr_in *addr4 = (struct sockaddr_in *)addr;
    if (!inet_ntop(AF_INET, &(addr4->sin_addr), addrstr,
                   INET6_ADDRSTRLEN + 1)) {
      logexit("ntop");
    }
    port = ntohs(addr4->sin_port);
  } else if (addr->sa_family == AF_INET6) {
    version = 6;
    struct sockaddr_in6 *addr6 = (struct sockaddr_in6 *)addr;
    if (!inet_ntop(AF_INET6, &(addr6->sin6_addr), addrstr,
                   INET6_ADDRSTRLEN + 1)) {
      logexit("ntop");
    }
    port = ntohs(addr6->sin6_port);
  } else {
    logexit("unknown protocol family");
  }
  if (str) {
    snprintf(str, strsize, "IPv%d %s %hu", version, addrstr, port);
  }
}

int server_sockaddr_init(const char *proto, const char *portstr,
                         struct sockaddr_storage *storage) {
  __uint16_t port = (__uint16_t)atoi(portstr);
  if (port == 0) {
    return -1;
  }
  port = htons(port);

  memset(storage, 0, sizeof(*storage));
  if (strcmp(proto, "v4") == 0) {
    struct sockaddr_in *addr4 = (struct sockaddr_in *)storage;
    addr4->sin_family = AF_INET;
    addr4->sin_port = port;
    addr4->sin_addr.s_addr = INADDR_ANY;
    return 0;
  } else if (strcmp(proto, "v6") == 0) {
    struct sockaddr_in6 *addr6 = (struct sockaddr_in6 *)storage;
    addr6->sin6_family = AF_INET6;
    addr6->sin6_port = port;
    addr6->sin6_addr = in6addr_any;
    return 0;
  } else {
    return -1;
  }
}

void start(CampoMinado *campoMinado, char *nomeArquivo) {
  FILE *arquivo;
  char linha[20];
  char *token;
  int linhaAtual = 0;

  arquivo = fopen(nomeArquivo, "r");

  if (arquivo == NULL) {
    perror("Erro ao abrir o arquivo");
    exit(1);
  }

  while (fgets(linha, sizeof(linha), arquivo) && linhaAtual < 4) {
    token = strtok(linha, ",");
    int colunaAtual = 0;

    while (token != NULL && colunaAtual < 4) {
      campoMinado->resposta[linhaAtual][colunaAtual] = atoi(token);
      campoMinado->estado[linhaAtual][colunaAtual] = HIDDEN;
      token = strtok(NULL, ",");
      colunaAtual++;
    }

    linhaAtual++;
  }

  fclose(arquivo);
}

void reveal(int *coordinates, CampoMinado *campoMinado) {
  campoMinado->estado[coordinates[0]][coordinates[1]] =
      campoMinado->resposta[coordinates[0]][coordinates[1]];
}

void flag(int *coordinates, CampoMinado *campoMinado) {
  campoMinado->estado[coordinates[0]][coordinates[1]] = FLAG;
}

void remove_flag(int *coordinates, CampoMinado *campoMinado) {
  campoMinado->estado[coordinates[0]][coordinates[1]] = HIDDEN;
}

void reset(CampoMinado *campoMinado) {
  for (int i = 0; i < 4; i++) {
    for (int j = 0; j < 4; j++) {
      campoMinado->estado[i][j] = HIDDEN;
    }
  }
}

void setCoordinates(int *coordinates) {
  char input[10];
  scanf("%s", input);
  sscanf(input, "%d,%d", &coordinates[0], &coordinates[1]);
}

Action setActionClient() {
  Action act;
  char type[20];
  int coordinates[2] = {-1, -1};

  scanf("%s", type);

  if (strcmp(type, "start") == 0) {
    act.type = htonl(0);
  } else if (strcmp(type, "reveal") == 0) {
    act.type = htonl(1);
    setCoordinates(coordinates);
  } else if (strcmp(type, "flag") == 0) {
    act.type = htonl(2);
    setCoordinates(coordinates);
  } else if (strcmp(type, "remove_flag") == 0) {
    act.type = htonl(4);
    setCoordinates(coordinates);
  } else if (strcmp(type, "reset") == 0) {
    act.type = htonl(5);
  } else if (strcmp(type, "exit") == 0) {
    act.type = htonl(7);
  } else if (strcmp(type, "game_over") == 0) {
    act.type = htonl(8);
  } else {
    act.type = htonl(-1);
  }
  act.coordinates[0] = htonl(coordinates[0]);
  act.coordinates[1] = htonl(coordinates[1]);
  return act;
}

void printEstado(CampoMinado *campoMinado) {
  for (int i = 0; i < 4; i++) {
    for (int j = 0; j < 4; j++) {
      if (campoMinado->estado[i][j] == BOMB) {
        printf("*");
      } else if (campoMinado->estado[i][j] == HIDDEN) {
        printf("-");
      } else if (campoMinado->estado[i][j] == FLAG) {
        printf(">");
      } else {
        printf("%d", campoMinado->estado[i][j]);
      }
      if (j != 3) {
        printf("\t\t");
      }
    }
    printf("\n");
  }
}