#ifndef traders_rating_utilities_h
#define traders_rating_utilities_h

#include <ctime>
#include <utility>

namespace traders_rating {

std::pair<time_t, time_t> get_week_times(time_t ts);
std::pair<time_t, time_t> get_minute_times(time_t ts);
void yield_thread();

}  // namespace traders_rating

#endif  // traders_rating_utilities_h