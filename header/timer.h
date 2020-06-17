#ifndef TIMER_H
#define TIMER_H

#include <time.h>

class cTimer {
  private:
	timespec process_start, frame_start, current;
  protected:
  public:
	cTimer();
	~cTimer();
	double elapsed(bool frame);
};

#endif