#include <common.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <unistd.h>

#define BUFSZ 1024

int main(int argc, char **argv) {
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

  char buf[BUFSZ];
  memset(buf, 0, BUFSZ);
  printf("mensagem> ");
  fgets(buf, BUFSZ - 1, stdin);
  
  int count = send(s, buf, strlen(buf) + 1, 0);

  if (count != strlen(buf) + 1) {
    logexit("send");
  }

  memset(buf, 0, BUFSZ);
  unsigned total = 0;
  while (1) {
    count = recv(s, buf + total, BUFSZ - total, 0);
    if (count == 0) {
      break;
    }
    total += count;
  }
  close(s);

  printf("received %d bytes\n", total);
  puts(buf);

  exit(1);

  return 0;
}