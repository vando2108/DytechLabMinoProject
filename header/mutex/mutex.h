#ifndef __MUTEX_H__
#define __MUTEX_H__
#include <pthread.h>

#include "../cond/cond.h"
#include "../utils/utils.h"

namespace Pthread {
  class IMutex {
    public:
      virtual ~IMutex(){};
  
      virtual int lock() = 0;
      virtual int trylock() = 0;
      virtual int unlock() = 0;
  };

  class CMutex: public IMutex, private IUncopyable {
    friend class CCond;
    public:
      CMutex();
      ~CMutex();
      inline int lock();
      inline int trylock();
      inline int unlock();
    private:
      pthread_mutex_t m_mutex;
  };

  class CGuard {
    public:
      CGuard(IMutex &mutex): m_mutex(mutex){m_mutex.lock();};
      ~CGuard(){m_mutex.unlock();};
    private:
      IMutex &m_mutex;
  };
}
#endif
