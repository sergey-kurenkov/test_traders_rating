#ifndef TR_UTILITIES_GET_TIME_H
#define TR_UTILITIES_GET_TIME_H

#include <ctime>
#include <utility>

namespace traders_rating {

std::pair<time_t, time_t> get_week_times(time_t ts);
std::pair<time_t, time_t> get_minute_times(time_t ts);

}  // namespace traders_rating

#endif  // TR_UTILITIES_GET_TIME_H