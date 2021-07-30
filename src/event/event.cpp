#include <unistd.h>
#include <assert.h>
#include <sys/socket.h>

#include "../../header/event/event.h"
#include "../../header/log/log.h"
#include "../../header/utils/utils.h"

namespace Event{
  int IEventHandle::register_event(int fd, EventType type) {
    return CEventProxy::instance()->register_event(fd, this, type);
  }

  int IEventHandle::register_event(Socket::ISocket &socket, EventType type) {
    return CEventProxy::instance()->register_event(socket, this, type);
  }

  int IEventHandle::shutdown_event(int fd) {
    return CEventProxy::instance()->shutdown_event(fd);
  }

  int IEventHandle::shutdown_event(Socket::ISocket &socket) {
    return CEventProxy::instance()->shutdown_event(socket);
  }

  CNetObserver::CNetObserver(INetObserver &obj, EventType regEvent):m_regEvent(regEvent), m_obj(obj), m_refcount(1) {}
  CNetObserver::~CNetObserver(){};

  void CNetObserver::addref() {
    Pthread::CGuard guard(m_refcount_mutex);
    m_refcount++;
  }

  inline void CNetObserver::subref() {
    Pthread::CGuard guard(m_refcount_mutex);
    m_refcount--;
  }

  bool CNetObserver::subref_and_test() {
    Pthread::CGuard guard(m_refcount_mutex);
    m_refcount--;
    return (m_refcount == 0x00);
  }

  void CNetObserver::selfrelease() {
    delete this;
  }

  EventType CNetObserver::get_regEvent() {
    return m_regEvent;
  }

  const INetObserver *CNetObserver::get_handle() {
    return &m_obj;
  }

  void CNetObserver::handle_in(int fd) {
    m_obj.handle_in(fd);
  }

  void CNetObserver::handle_out(int fd) {
    m_obj.handle_out(fd);
  }

  void CNetObserver::handle_close(int fd) {
    m_obj.handle_close(fd);
  }

  void CNetObserver::handle_error(int fd) {
    m_obj.handle_error(fd);
  }

  CEvent::CEvent(size_t event_max) {
    m_detectionThread = 0;
    bzero((void *)&m_eventBuff, sizeof(m_eventBuff));

    m_epollfd = epoll_create(event_max);
    assert(m_epollfd >= 0);

    m_ithreadpool = ThreadPool::CThreadPoolProxy::instance();

    pthread_t tid = 0;
    if (pthread_create(&tid, NULL, eventwait_thread, (void *)this) == 0)
      m_detectionThread = tid;
  }

  CEvent::~CEvent() {
    if (m_detectionThread != 0 && pthread_cancel(m_detectionThread) == 0) { 
      pthread_join(m_detectionThread, (void **) NULL);
    }

    if (!INVALID_FD(m_epollfd)) {
      close(m_epollfd);
    }
  }

  int CEvent::register_event(int fd, IEventHandle *handle, EventType type) {
    if (INVALID_FD(fd) || INVALID_FD(m_epollfd) || INVALID_POINTER(handle)) {
      seterrno(EINVAL);
      return -1;
    }

    struct epoll_event newEvent;
    newEvent.data.fd = fd;
    newEvent.events = type;

    ExistRet ret = isExist(fd, type, handle);
    if (ret == Existed) {
      return 0;
    }

    record(fd, type, handle);
    if (ret == HandleModify) {
      return 0;
    }

    int opt;
    if (ret == TypeModify || ret == Modify) 
      opt = EPOLL_CTL_MOD;
    else if (ret == NotExist) 
      opt = EPOLL_CTL_ADD;
    
    if (epoll_ctl(m_epollfd, opt, fd, &newEvent) < 0) {
      errsys("event center 124: epoll op %d, fd %#x error\n", opt, fd);
      detach(fd, true);
      return -1;
    }

    return 0;
  }

  int CEvent::unregister_event(int fd) {
    if (epoll_ctl(m_epollfd, EPOLL_CTL_DEL, fd, NULL) < 0) {
      errsys("event center 134: failed to delete epoll %d", fd);
      return -1;
    }
    return detach(fd);
  }

  int CEvent::shutdown_event(int fd) {
    trace("sock[%#X] shutdown event\n", fd);
    return ::shutdown(fd, SHUT_WR);
  }

  CEvent::ExistRet CEvent::isExist(int fd, EventType type, IEventHandle *handle) {
    Pthread::CGuard guard(m_eventReg_mutex);
    auto it = m_eventReg.find(fd);
    if (it == m_eventReg.end()) {
      return NotExist;
    }
    
    CNetObserver &eventHandle = *(it->second);
    EventType oldRegType = eventHandle.get_regEvent();
    const INetObserver *oldRegHandle = eventHandle.get_handle();
    
    if (oldRegType != type && oldRegHandle != handle) 
      return Modify;
    else if (oldRegType != type) 
      return TypeModify;
    else if (oldRegHandle != handle) {
      return HandleModify;
    }

    return Existed;
  }

  int CEvent::record(int fd, EventType type, IEventHandle *handle) {
    CNetObserver *newObServer = new CNetObserver(*handle, type);
    assert(newObServer != NULL);

    Pthread::CGuard guard(m_eventReg_mutex);
    m_eventReg[fd] = newObServer;
    return 0;
  }

  int CEvent::detach(int id, bool release) {
    Pthread::CGuard guard(m_eventReg_mutex);
    auto it = m_eventReg.find(id);
    if (it == m_eventReg.end()) {
      return -1;
    }

    if (release) 
      it->second->selfrelease();

    m_eventReg.erase(it);
    return 0;
  }

  CNetObserver *CEvent::get_observer(int fd) {
    Pthread::CGuard guard(m_eventReg_mutex);
    auto it = m_eventReg.find(fd);
    if (it == m_eventReg.end()) 
      return NULL;
    
    CNetObserver *observer = m_eventReg[fd];
    observer->addref();
    return observer;
  }

  int CEvent::pushtask(int fd, EventType type) {
    Pthread::CGuard guard(m_events_mutex);
    auto it = m_events.find(fd);
    if (it == m_events.end()) {
      m_events[fd] = type;
      return 0;
    }
    it->second = (EventType)(it->second | type);
    return 0;
  }

  int CEvent::poptask(int &fd, EventType &type) {
    Pthread::CGuard guard(m_events_mutex);
    auto it = m_events.begin();
    if (it == m_events.end()) {
      return -1; //dose not exist
    }
    fd = it->first;
    type = it->second;
    m_events.erase(it);
    return 0;
  }

  int CEvent::cleartask(int fd) {
    //fd == -1 clear all task
    if (fd == -1) {
      m_events.clear();
      return 0;
    }
    else if (fd >= 0) {
      auto it = m_events.find(fd);
      if (it == m_events.end())
	return -1;
      m_events.erase(it);
      return 0;
    }
    return -1;
  }
  
  size_t CEvent::tasksize() {
    Pthread::CGuard guard(m_events_mutex);
    return m_events.size();
  }

  void CEvent::threadhandle() {
    int fd = 0x00;
    EventType event;
    if (poptask(fd, event) < 0)
      return;

    CNetObserver *observer = get_observer(fd);
    if (observer == NULL)
      return;

    if (event & ECLOSE) {
      cleartask(fd);
      observer->subref();
    }
    else {
      if (event & EERR)
	observer->handle_error(fd);
      if (event & EIN)
	observer->handle_in(fd);
      if (event & EOUT)
	observer->handle_out(fd);
    }

    if (observer->subref_and_test()) {
      unregister_event(fd);
      observer->handle_close(fd);
      observer->selfrelease();
    }
  }

  void *CEvent::eventwait_thread(void *arg) {
    CEvent &cEvent = *(CEvent *)(arg);
    if (INVALID_FD(cEvent.m_epollfd)) {
      seterrno(EINVAL);
      pthread_exit(NULL);
    }
    
    while (true) {
      int nEvent = epoll_wait(cEvent.m_epollfd, &cEvent.m_eventBuff[0], EventBuffLen, -1);
      if (nEvent < 0 && errno != EINTR) {
	errsys("event Center 285: epoll wait error\n");
	break;
      }

      for (int i = 0; i < nEvent; i++) {
	int fd = cEvent.m_eventBuff[i].data.fd;
	EventType event = static_cast<EventType>(cEvent.m_eventBuff[i].events);
	
	if (cEvent.pushtask(fd, event) == 0x00) {
	  cEvent.m_ithreadpool->pushtask(&cEvent);
	}
      }
    }

    pthread_exit(NULL);
  }

  CEventProxy::CEventProxy(size_t event_max) {
    m_event = new CEvent(event_max);
  }

  CEventProxy::~CEventProxy() {
    if (m_event) delete m_event;
  }

  CEventProxy *CEventProxy::instance() {
    static CEventProxy *eventProxy = NULL;
    if (eventProxy == NULL) {
      eventProxy = new CEventProxy(NEVENT_MAX);
    }
    return eventProxy;
  }

  int CEventProxy::register_event(int fd, IEventHandle *handle, EventType type) {
    return m_event->register_event(fd, handle, type);
  }

  int CEventProxy::register_event(Socket::ISocket &socket, IEventHandle *handle, EventType type) {
    return register_event(socket.fd(), handle, type);
  }

  int CEventProxy::shutdown_event(int fd) {
    return m_event->shutdown_event(fd);
  }

  int CEventProxy::shutdown_event(Socket::ISocket &socket) {
    return m_event->shutdown_event(socket.fd());
  }
}
