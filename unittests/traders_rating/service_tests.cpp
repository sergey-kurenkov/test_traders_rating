#include "gtest/gtest.h"

#include "traders_rating/cmds.h"
#include "traders_rating/service.h"
#include "traders_rating/utilities.h"

namespace tr = ::traders_rating;

struct test_get_rating_result {
	test_get_rating_result() {
		using std::placeholders;
		callback = (std::bind(&test_get_rating_result::upload, this, _1));
	}
	void upload(rating_result_t& arg) {
		res = arg;
	}
	tr::rating_result_t res;
	tr::rating_result_callback callback;
};

TEST(ServiceTest, Create) {
	try {
		test_get_rating_result result;
		tr::service srv(result.callback);
	} catch(std::exception& e) {
		FAIL() << e.what();
	}
}

TEST(ServiceTest, DISABLED_StartStop) {
	try {
		test_get_rating_result result;
		tr::service srv(result.callback);
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

TEST(MinuteRatingTest, OnUserDealWon) {
	try {
		auto ts = time(nullptr);
		auto minute_ts = tr::get_minute_times(ts);
		tr::minute_rating rating(minute_ts.first, minute_ts.second);
		rating.on_user_deal_won(ts, 100, 105.1);
		ASSERT_EQ((*rating.begin()).first, 100);
		ASSERT_EQ((*rating.begin()).second, 105.1);
	} catch(std::exception& e) {
		FAIL() << e.what();
	}
}

TEST(WeekRatingTest, Create) {
	try {
		auto ts = time(nullptr);
		auto week_ts = tr::get_week_times(ts);
		test_get_rating_result result;

		tr::week_rating rating(week_ts.first, week_ts.second, 
			[](std::vector<tr::user_id_t>&){
				
			}, result.callback);
	} catch(std::exception& e) {
		FAIL() << e.what();
	}
}

struct WeekRatingHelper : public ::testing::Test {
	WeekRatingHelper() {}
	void create_rating(time_t ts) {
		start_ts = ts;
		week_ts = tr::get_week_times(start_ts);
		callback = [](std::vector<tr::user_id_t>&) {

		};
		rating.reset(new tr::week_rating(week_ts.first, week_ts.second, callback, result.callback));
	}

	time_t start_ts;
	std::pair<time_t, time_t> week_ts;
	tr::get_connected_callback callback;
	test_get_rating_result result;
	std::unique_ptr<tr::week_rating> rating; 
};

TEST_F(WeekRatingHelper, StartStop) {
	try {
		auto ts = time(nullptr);
		create_rating(ts);
		rating->start();
		rating->stop();
	} catch(std::exception& e) {
		FAIL() << e.what();
	}
}
