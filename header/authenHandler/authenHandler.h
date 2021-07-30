#include <stdio.h>
#include <map>
#include <string>

#include "../mutex/mutex.h"

namespace AuthenHandler {
  #define DB_FILE 	"USER.db" 

  class CAuthen {    
    public: 
      static CAuthen *instance();

      bool login(const std::string &username, const std::string &password);
      bool  addNewuser(const std::string &username, const std::string &password);  

    private:
      CAuthen();
      ~CAuthen();

      std::map<std::string, std::string> m_usersList; //key is username, value is password
      Pthread::CMutex m_authen_mutex;
  };
}
