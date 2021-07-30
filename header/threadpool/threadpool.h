#ifndef __THREADPOOL_H__
#define __THREADPOOL_H__

#include <pthread.h>

#include <vector>

#include "../queue/queue.h"

namespace ThreadPool {
  class IThreadHandle {
    friend class CThreadPool;
    public:
      virtual ~IThreadHandle(){};
    	
    protected:
      virtual void threadhandle() = 0;
  };

  class IThreadPool {
    public:
      virtual ~IThreadPool(){};

      virtual int pushtask(IThreadHandle *handle, bool block = true) = 0;
  };

  class CThreadPool: public IThreadPool {
    public:
      enum Limit {
	THREADNUM_MAX = 64,
	TASKNUM_MAX = 2018,
      };

      typedef void *(threadproc_t)(void *);
      typedef std::vector<pthread_t> vector_tid_t;
      typedef Queue::CQueue<IThreadHandle *> queue_handle_t;
		
    public:
      CThreadPool(size_t threadnum, size_t tasknum);
      ~CThreadPool();
      int pushtask(IThreadHandle *handle, bool block);
		
    private:
      void promote_leader();
      void join_follwer();
      void create_threadpool();
      void destroy_threadpool();
      static void *process_task(void *arg);
		
    private:
      size_t m_threadnum;
      vector_tid_t m_thread;
      
      queue_handle_t m_taskqueue;
      
      bool m_hasleader;
      Pthread::CCond m_befollower_cond;
      Pthread::CMutex m_identify_mutex;
  };

  class CThreadPoolProxy: public IThreadPool {
    private:
      IThreadPool *m_ithreadpool;

    public:
      static IThreadPool *instance() {
	static CThreadPoolProxy threadpoolproxy;
	return &threadpoolproxy;
      }

    int pushtask(IThreadHandle * handle, bool block) { 
    	return m_ithreadpool->pushtask(handle, block);
    }
		
    private:
      CThreadPoolProxy() { 
      	m_ithreadpool = new CThreadPool(4, 1024);
      }

      ~CThreadPoolProxy() { 
      	delete m_ithreadpool;
      }
  };
}

#endif
