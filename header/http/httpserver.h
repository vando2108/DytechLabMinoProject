#ifndef __HTTPSERVER_H__
#define __HTTPSERVER_H__

#include "../ISocket/server.h"
#include "../event/event.h"
#include "./http.h"

namespace Http {
  #define HTTP_ROOT 	     "templates"
  #define HTTP_DEFAULTFILE   "index.html"
  #define HTTP_SLASH     	"/"
  #define HTTP_CURRENT_DIR  "."
  #define HTTP_ABOVE_DIR     ".."

  
  std::string http_path_handle(const std::string &dname, const std::string &bname);
  inline void split_url(const std::string &url, std::string &dir, std::string &base);
  class HttpStream: public Event::IEventHandle {
    public:
      HttpStream(Socket::IClient *);
      ~HttpStream();
      int close();
      
    protected:
      void handle_in(int fd);
      void handle_out(int fd);
      void handle_close(int fd);
      void handle_error(int fd);
      
    private:
      Http::IHttpRespose *handle_request(Http::IHttpRequest &request);

    private:
      enum CONFIG {
      	READBUFF_LEN = 1024,
      };

    private:
      Socket::IClient *m_client;
  
      Pthread::CMutex m_readBuffmutex;
      char *m_readBuff;
      
  };
  
  class HttpServer: public Event::IEventHandle {
    public:
      HttpServer(const std::string &addr, uint16_t port);
      ~HttpServer();
      int start(size_t backlog);
      int close();
      
    protected:
      void handle_in(int fd);
      void handle_out(int fd);
      void handle_close(int fd);
      void handle_error(int fd);
      
    private:
      Pthread::CMutex m_sockmutex;
      uint16_t m_port;
      const std::string m_addr;
      Socket::CStreamServer m_server;
  };

  class CAuthen {    
    public: 
      static CAuthen *instance();

      bool login(std::string username, std::string password);
      bool  addNewuser(std::string username, std::string password);  

    private:
      CAuthen();
      ~CAuthen();

      std::map<std::string, std::string> m_usersList; //key is username, value is password
      Pthread::CMutex m_authen_mutex;
  };
}

#endif
