#include "gtest/gtest.h"

#include "traders_rating/cmds.h"
#include "traders_rating/service.h"
#include "traders_rating/utilities.h"

namespace tr = ::traders_rating;


TEST(GetMinuteTimeTest, Test1)  {
	try {
		time_t ts = time(nullptr);
		tm ts_tm = *localtime(&ts);
		auto res = tr::get_minute_times(ts);
		ASSERT_EQ(res.first, ts - ts_tm.tm_sec);
		ASSERT_EQ(res.second, ts - ts_tm.tm_sec + 60);
	} catch(std::exception& e) {
		FAIL() << e.what();
	}
}

TEST(GetWeekTimeTest, Test1)  {
	try {
		time_t ts = time(nullptr);
		tm ts_tm = *localtime(&ts);
		auto curr_day = ts_tm.tm_wday == 0 ? 6 : ts_tm.tm_wday - 1;
		auto res = tr::get_week_times(ts);
		ASSERT_EQ(res.first, ts - ts_tm.tm_sec - ts_tm.tm_min*60 - ts_tm.tm_hour*3600 - curr_day * 24 * 3600);
	} catch(std::exception& e) {
		FAIL() << e.what();
	}
}