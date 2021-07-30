#ifndef  __COND_H__
#define  __COND_H__

#include <pthread.h>

namespace Pthread {
  class CMutex;
  class ICond {
    public:
      virtual ~ICond(){};
      virtual int wait(CMutex &) = 0;
      virtual int signal() = 0;
      virtual int broadcast() = 0;
  };

  class CCond: public ICond {
    public:
      CCond();
      ~CCond();
      int wait(CMutex &);
      int signal();
      int broadcast();

    private:
      pthread_cond_t m_cond;
  };
}

#endif
