#include "traders_rating/utilities.h"

#include <ctime>
#include <utility>
#include <cassert>
#include <string>
#include <thread>

#include <time.h>
#if defined __linux__
#include <pthread.h>
#endif

namespace tr = ::traders_rating;

std::pair<time_t, time_t> tr::get_week_times(time_t ts) {
  tm this_ts;
  localtime_r(&ts, &this_ts);
  time_t start = ts - this_ts.tm_sec - this_ts.tm_min * 60 -
                 this_ts.tm_hour * 3600 -
                 (this_ts.tm_wday == 0 ? 6 : this_ts.tm_wday - 1) * (24 * 3600);

  tm this_week;
  localtime_r(&start, &this_week);
  assert(this_week.tm_sec == 0);
  assert(this_week.tm_min == 0);
  assert(this_week.tm_hour == 0);
  assert(this_week.tm_wday == 1);

  time_t next_week_start = start + 24 * 7 * 3600;
  tm next_week;
  localtime_r(&next_week_start, &next_week);
  assert(next_week.tm_sec == 0);
  assert(next_week.tm_min == 0);
  assert(next_week.tm_hour == 0);
  assert(next_week.tm_wday == 1);
  assert(next_week.tm_yday != this_week.tm_yday);
  return std::make_pair(start, next_week_start);
}

std::pair<time_t, time_t> tr::get_minute_times(time_t ts) {
  tm this_ts;
  localtime_r(&ts, &this_ts);
  time_t start = ts - this_ts.tm_sec;
  ;
  time_t next_minute_start = start + 60;
  return std::make_pair(start, next_minute_start);
}

void tr::yield_thread() { 
#if defined __linux__
  pthread_yield(); 
#else
  std::this_thread::yield();
#endif
}