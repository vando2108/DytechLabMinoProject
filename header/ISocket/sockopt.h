#ifndef __SOCKOPT_H__
#define __SOCKOPT_H__

#include <string>
#include <stdint.h>
#include <netinet/in.h>

namespace Socket {
  int set_resuseport(int sockfd, bool en);
  int set_nonblock(int sockfd, bool en);
  int convert_inaddr(const std::string &addr, uint16_t port, struct sockaddr_in &sockaddr);
}

#endif
