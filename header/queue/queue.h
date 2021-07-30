#ifndef __QUEUE_H__
#define __QUEUE_H__

#include <queue>

#include "../mutex/mutex.h"

namespace Queue {
  template<typename T> class IQueue {
      public:
	virtual ~IQueue(){};
  	
  	virtual size_t size() = 0;
  	virtual int clear() = 0;
  	virtual int pop(T &out) = 0;
  	virtual int pop_nonblock(T &out) = 0;
  	virtual int push(T &in) = 0;
  	virtual int push_nonblock(T &in) = 0;
  };

  template<typename TYPE> class CQueue: public IQueue<TYPE> {
    private:
      typedef std::queue<TYPE> queue_t;
      queue_t m_queue;
      Pthread::CMutex m_queuemutex;
      Pthread::CCond m_queuefill;
      bool m_waitqueuefill;
      Pthread::CCond m_queuefree;
      bool m_waitqueuefree;
    
    	const size_t m_queuelen;
    public:
      CQueue(size_t queuelen): m_waitqueuefill(false), m_queuelen(queuelen){};
      ~CQueue(){};

      size_t size() {
      	Pthread::CGuard guard(m_queuemutex);
      	return (size_t)m_queue.size();
      }

      int clear() {
      	Pthread::CGuard guard(m_queuemutex);
      	while(!m_queue.empty())
      	  m_queue.pop();
      	return 0;
      }

      int pop_nonblock(TYPE &out) {
      	Pthread::CGuard guard(m_queuemutex);
      	if(m_queue.size() == 0)
      	  return -1;
      
      	out = m_queue.front();
      	m_queue.pop();
      
      	if(m_waitqueuefree) {
	  m_queuefree.signal();
      	  m_waitqueuefree = false;
      	}
      
      	return 0;
      }

      int pop(TYPE &out) {
      	Pthread::CGuard guard(m_queuemutex);
      	while(m_queue.size() == 0){
	  m_waitqueuefill = true;
      	  m_queuefill.wait(m_queuemutex);
      	}
      
      	out = m_queue.front();
      	m_queue.pop();
      
      	if(m_waitqueuefree) {
      	  m_queuefree.signal();
      	  m_waitqueuefree = false;
      	}

      	return 0;
      }

      int push_nonblock(TYPE &in) {
      	Pthread::CGuard guard(m_queuemutex);
      	if(m_queue.size() == m_queuelen)
      	  return -1;
      
      	m_queue.push(in);
      
      	if(m_waitqueuefill) {
      	  m_queuefill.signal();
      	  m_waitqueuefill = false;
      	}

      	return 0;
      }

      int push(TYPE &in) {
      	Pthread::CGuard guard(m_queuemutex);
      	while(m_queue.size() == m_queuelen){
      	  m_waitqueuefree = true;
      	  m_queuefree.wait(m_queuemutex);
      	}
      
      	m_queue.push(in);
      
      	if(m_waitqueuefill) {
      	  m_queuefill.signal();
      	  m_waitqueuefill = false;
      	}

      	return 0;
      };		
  };
}
#endif
