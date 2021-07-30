#include "../../header/mutex/mutex.h"
#include "../../header/authenHandler/authenHandler.h"

namespace AuthenHandler {
  CAuthen::CAuthen() {}
  CAuthen::~CAuthen() {}

  CAuthen *CAuthen::instance() {
    static CAuthen *authen = NULL;
    if (authen == NULL) 
      authen = new CAuthen();
    return authen;
  }

  bool CAuthen::login(const std::string &username, const std::string &password) {
    Pthread::CGuard guard(m_authen_mutex);
    
    auto it = m_usersList.find(username);
    if (it == m_usersList.end())
      return false;

    return (it->second == password);
  }

  bool CAuthen::addNewuser(const std::string &username, const std::string &password) {
    Pthread::CGuard guard(m_authen_mutex);

    auto it = m_usersList.find(username);
    if (it != m_usersList.end())
      return false;;

    m_usersList[username] = password;

    return true;
  }
}
