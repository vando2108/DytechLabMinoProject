#ifndef __EVENT_H__
#define __EVENT_H__

#include <sys/epoll.h>

#include <map>

#include "../mutex/mutex.h"
#include "../queue/queue.h"
#include "../ISocket/ISocket.h"
#include "../threadpool/threadpool.h"

namespace Event {
  enum EventConfig {
    NEVENT_MAX = 1024,  
  };

  enum EventType {
    EIN = EPOLLIN,
    EOUT = EPOLLOUT,
    ECLOSE = EPOLLRDHUP,
    EPRI = EPOLLPRI,
    EERR = EPOLLERR,
    EET = EPOLLET, 
    EDEFULT = EIN | ECLOSE | EERR | EET
  };
  
  class INetObserver {
    friend class CNetObserver;
    public:
      virtual ~INetObserver(){};
     
    protected:
      virtual void handle_in(int) = 0;
      virtual void handle_out(int) = 0;
      virtual void handle_close(int) = 0;
      virtual void handle_error(int) = 0;
  };
  
  class IEventHandle: public INetObserver {
    public:
      int register_event(int fd, EventType type = EDEFULT);
      int register_event(Socket::ISocket &socket, EventType type = EDEFULT);
      int shutdown_event(int fd);
      int shutdown_event(Socket::ISocket &);
  };

  class CNetObserver: public INetObserver {
    friend class CEvent;
    public:
      CNetObserver(INetObserver &, EventType);
      ~CNetObserver();
      inline void addref();
      inline void subref();
      inline bool subref_and_test();
      inline void selfrelease();
      inline EventType get_regEvent();
      inline const INetObserver *get_handle();
     
    protected:
      void handle_in(int);
      void handle_out(int);
      void handle_close(int);
      void handle_error(int);
     
    private:
      EventType m_regEvent;
      INetObserver &m_obj;
      
      int32_t m_refcount;
      Pthread::CMutex m_refcount_mutex;
  };
  
  class IEvent {  
    public:
      virtual ~IEvent(){};
      
      virtual int register_event(int, IEventHandle *, EventType) = 0;
      virtual int shutdown_event(int) = 0;
  };

  class CEvent: public IEvent, public ThreadPool::IThreadHandle {
    public:
      CEvent(size_t event_max);
      ~CEvent();
      int register_event(int, IEventHandle *, EventType);
      int shutdown_event(int);
    
    protected:
      void threadhandle();
    
    private:
      enum ExistRet {
        NotExist, 
        HandleModify, 
        TypeModify, 
        Modify, 
        Existed,
      };
  
      enum Limit {
       EventBuffLen = 1024, CommitAgainNum = 2,
      };
      
      typedef std::map<int, CNetObserver *> EventMap_t;
      typedef std::map<int, EventType> EventTask_t;
    
    private:
      ExistRet isExist(int fd, EventType type, IEventHandle *handle);
      int record(int fd, EventType eventtype, IEventHandle *handle);
      int detach(int fd, bool release = false);
      CNetObserver *get_observer(int fd);
      int pushtask(int fd, EventType event);
      int poptask(int &fd, EventType &event);
      size_t tasksize();
      int cleartask(int fd);
      int unregister_event(int);
      static void *eventwait_thread(void *arg);
    
    private:
      int m_epollfd;
      EventMap_t m_eventReg;
      Pthread::CMutex m_eventReg_mutex;
      EventTask_t m_events;
      Pthread::CMutex m_events_mutex;
      
      struct epoll_event m_eventBuff[EventBuffLen];
      
      pthread_t m_detectionThread;
      
      ThreadPool::IThreadPool *m_ithreadpool;
  };
  
  class CEventProxy: public IEvent {
    public:
      static CEventProxy *instance(); 
      int register_event(int, IEventHandle *,EventType);
      int register_event(Socket::ISocket &, IEventHandle *,EventType);
      int shutdown_event(int);
      int shutdown_event(Socket::ISocket &);
    
    private:
      CEventProxy(size_t neventmax);
      ~CEventProxy();
    
    private:
      IEvent *m_event;
  };

}
#endif
