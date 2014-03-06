#ifndef COMMON_HH
#define COMMON_HH

#include <sys/time.h>

enum
{ 
    MSG_NONE   = 0x00,
    MSG_DOWN   = 0x01,
    MSG_UP     = 0x02,
    MSG_LEFT   = 0x04,
    MSG_RIGHT  = 0x08,
    MSG_FIRE   = 0x10,
    MSG_STOP   = 0x20,
    MSG_STATUS = 0x40,
};

struct Timer
{
  timeval tv;
  void update()    {gettimeofday( &tv, 0);}
  double toDouble(){return tv.tv_sec + tv.tv_usec*1e-6;}
};

#endif
