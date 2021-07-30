#ifndef __PARAMCHECK_H__
#define __PARAMCHECK_H__

#include <errno.h>

#define INVALID_FD(fd) (fd < 0)
#define INVALID_POINTER(p) (p == NULL)

class IUncopyable {
  public:
    IUncopyable(){};

  private:
    IUncopyable(IUncopyable &);
    IUncopyable & operator=(const IUncopyable&);
};

inline void seterrno(int eno) {
  errno = eno;
}

#endif
