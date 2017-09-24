#include "gtest/gtest.h"

#include "traders_rating/cmds.h"
#include "traders_rating/service.h"

namespace tr = ::traders_rating;


TEST(GetMinuteTimeTest, Test1)  {
	try {
		time_t ts = time(nullptr);
		tm ts_tm = *localtime(&ts);
		auto res = tr::get_minute_times(ts);
		ASSERT_EQ(res.first, ts - ts_tm.tm_sec);
		ASSERT_EQ(res.first, ts - ts_tm.tm_sec + 60);
	} catch(std::exception& e) {
		FAIL() << e.what();
	}
}