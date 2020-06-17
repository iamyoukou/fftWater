#include "timer.h"

cTimer::cTimer() {
  clock_gettime(CLOCK_REALTIME, &process_start);
  frame_start = process_start;
}

cTimer::~cTimer() {}

double cTimer::elapsed(bool frame) {
  clock_gettime(CLOCK_REALTIME, &current);
  double elapsed =
      frame ? (current.tv_sec + current.tv_nsec / 1000000000.0 -
               frame_start.tv_sec - frame_start.tv_nsec / 1000000000.0)
            : (current.tv_sec + current.tv_nsec / 1000000000.0 -
               process_start.tv_sec - process_start.tv_nsec / 1000000000.0);
  frame_start = current;
  return elapsed;
}
