#ifndef __SERVER_H__
#define __SERVER_H__

#include <netinet/in.h>
#include <arpa/inet.h>
#include <string>

#include "../mutex/mutex.h"
#include "../ISocket/ISocket.h"

namespace Socket {
  class CStreamServer: public IServer {
    private:
      std::string m_addr;
      uint16_t m_port;

      int m_sockfd;
      Pthread::CMutex m_socketMutex;

    public:
      CStreamServer(const std::string &addr, uint16_t port);
      ~CStreamServer();

      int set_nonblock(bool nonblock);
      int fd();
      int start(size_t backlog);
      int close();
      bool isClose();
      IClient *accept();
  };
}
#endif
