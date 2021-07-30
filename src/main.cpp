#include <unistd.h>
#include <stdio.h>

#include "../header/log/log.h"
#include "../header/http/httpserver.h"

int main() {
  Http::HttpServer httpserver("127.0.0.1", 8080);
  if (httpserver.start(1024) < 0) {
    errsys("server create failed\n");
    return -1;
  }

  while (true) {
    char buff[1024];
    char *cmd = fgets(buff, sizeof(buff), stdin);
    if (cmd == NULL)
      break;
  }

  trace("terminate ...\n");
  return 0;
} 
