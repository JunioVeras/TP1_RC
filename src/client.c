#include <arpa/inet.h>
#include <common.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

int main(int argc, char **argv) {
  // criacao do socket e conexao com servidor
  struct sockaddr_storage storage;
  if (addrparse(argv[1], argv[2], &storage) != 0) {
    logexit("addr");
  }

  int s = socket(storage.ss_family, SOCK_STREAM, 0);
  if (s == -1) {
    logexit("socket");
  }

  struct sockaddr *addr = (struct sockaddr *)(&storage);
  if (connect(s, addr, sizeof(storage)) != 0) {
    logexit("connect");
  }

  char addrstr[BUFSZ];
  addrtostr(addr, addrstr, BUFSZ);

  printf("connected to %s\n", addrstr);

  while (1) {
    // envio de mensagens
    char buf[sizeof(Action)];
    memset(buf, 0, sizeof(Action));
    Action action = setActionClient();
    printAction(action);
    action = endianessSend(action);
    memcpy(buf, &action, sizeof(Action));

    int count = send(s, buf, sizeof(Action), 0);

    if (count != sizeof(Action)) {
      logexit("send");
    }

    // recebimento de mensagens
    memset(buf, 0, sizeof(Action));
    count = recv(s, buf, sizeof(Action), 0);
    memcpy(&action, buf, sizeof(Action));
    action = endianessRcv(action);
    printAction(action);
  }

  close(s);

  exit(1);

  return 0;
}