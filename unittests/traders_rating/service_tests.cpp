#include "gtest/gtest.h"

#include "traders_rating/cmds.h"
#include "traders_rating/service.h"
#include "traders_rating/utilities.h"

namespace tr = ::traders_rating;

TEST(ServiceTest, Create) {
	try {
		tr::service srv;
	} catch(std::exception& e) {
		FAIL() << e.what();
	}
}

TEST(ServiceTest, DISABLED_StartStop) {
	try {
		tr::service srv;
		srv.start();
		srv.stop();
	} catch(std::exception& e) {
		FAIL() << e.what();
	}
}

TEST(MinuteRatingTest, Create) {
	try {
		auto ts = time(nullptr);
		auto minute_ts = tr::get_minute_times(ts);
		tr::minute_rating rating(minute_ts.first, minute_ts.second);
	} catch(std::exception& e) {
		FAIL() << e.what();
	}
}