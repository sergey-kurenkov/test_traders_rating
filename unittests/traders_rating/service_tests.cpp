#include "gtest/gtest.h"

#include "traders_rating/cmds.h"
#include "traders_rating/service.h"
#include "traders_rating/utilities.h"

namespace tr = ::traders_rating;

struct test_get_rating_result {
  test_get_rating_result()
      : callback(std::bind(&test_get_rating_result::upload, this,
                           std::placeholders::_1)) {}
  void upload(const tr::rating_result_t& trading_result) {
    if (trading_results.count(trading_result.user_id) > 0) {
      ASSERT_FALSE(trading_results[trading_result.user_id].ts !=
                   trading_result.ts);
      trading_results.erase(trading_result.user_id);
    }
    trading_results[trading_result.user_id] = trading_result;
  }
  std::unordered_map<tr::user_id_t, tr::rating_result_t> trading_results;
  tr::upload_result_callback callback;
};

TEST(ServiceTest, Create) {
  try {
    test_get_rating_result result;
    tr::service srv(result.callback);
  }
  catch (std::exception& e) {
    FAIL() << e.what();
  }
}

TEST(ServiceTest, StartStop) {
  try {
    test_get_rating_result result;
    tr::service srv(result.callback);
    srv.start();
    srv.stop();
  }
  catch (std::exception& e) {
    FAIL() << e.what();
  }
}

TEST(MinuteRatingTest, Create) {
  try {
    auto ts = time(nullptr);
    auto minute_ts = tr::get_minute_times(ts);
    tr::minute_rating rating(minute_ts.first, minute_ts.second);
  }
  catch (std::exception& e) {
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
  }
  catch (std::exception& e) {
    FAIL() << e.what();
  }
}

TEST(WeekRatingTest, Create) {
  try {
    auto ts = time(nullptr);
    auto week_ts = tr::get_week_times(ts);
    test_get_rating_result result;

    tr::week_rating rating(week_ts.first, week_ts.second,
                           [](std::vector<tr::user_id_t>&) {}, result.callback);
    ASSERT_EQ(rating.started(), false);
    ASSERT_EQ(rating.finished(), false);
  }
  catch (std::exception& e) {
    FAIL() << e.what();
  }
}

struct WeekRatingFixture : public ::testing::Test {
  WeekRatingFixture() {}
  void create_rating() {
    start_ts = time(nullptr);
    week_ts = tr::get_week_times(start_ts);
    callback = [](std::vector<tr::user_id_t>&) {};
    rating.reset(new tr::week_rating(week_ts.first, week_ts.second, callback,
                                     result.callback));
  }

  void create_rating(time_t ts, tr::time_function_t time_function) {
    start_ts = ts;
    week_ts = tr::get_week_times(start_ts);
    callback = [](std::vector<tr::user_id_t>& connected) {
      connected.clear();
      connected.push_back(10);
      connected.push_back(20);
      connected.push_back(30);
      connected.push_back(40);
      connected.push_back(50);
    };
    rating.reset(new tr::week_rating(week_ts.first, week_ts.second, callback,
                                     result.callback, time_function));
  }

  time_t start_ts;
  std::pair<time_t, time_t> week_ts;
  tr::get_connected_callback callback;
  test_get_rating_result result;
  std::unique_ptr<tr::week_rating> rating;
};

TEST_F(WeekRatingFixture, StartStop) {
  try {
    create_rating();
    ASSERT_EQ(rating->started(), false);
    ASSERT_EQ(rating->finished(), false);
    rating->start();
    std::this_thread::sleep_for(std::chrono::seconds(1));
    ASSERT_EQ(rating->started(), true);
    ASSERT_EQ(rating->finished(), false);
    rating->stop();
    ASSERT_EQ(rating->started(), true);
    ASSERT_EQ(rating->finished(), true);
  }
  catch (std::exception& e) {
    FAIL() << e.what();
  }
}

TEST_F(WeekRatingFixture, StartAfterWeekStop) {
  try {
    auto start_ts = time(nullptr);
    auto time_function = [&](time_t*) {
      auto current_ts = time(nullptr);
      if (current_ts > start_ts + 2) {
        return start_ts + 7 * 24 * 3600;
      }
      return current_ts;
    };
    create_rating(start_ts, time_function);
    ASSERT_EQ(rating->started(), false);
    ASSERT_EQ(rating->finished(), false);
    rating->start();
    std::this_thread::sleep_for(std::chrono::seconds(1));
    ASSERT_EQ(rating->started(), true);
    ASSERT_EQ(rating->finished(), false);
    std::this_thread::sleep_for(std::chrono::seconds(3));
    ASSERT_EQ(rating->started(), true);
    ASSERT_EQ(rating->finished(), true);
    rating->stop();
    ASSERT_EQ(rating->started(), true);
    ASSERT_EQ(rating->finished(), true);
  }
  catch (std::exception& e) {
    FAIL() << e.what();
  }
}

TEST_F(WeekRatingFixture, OnMinute) {
  try {
    auto start_ts = time(nullptr);
    std::atomic_bool rating_posted(false);
    auto time_function = [&](time_t*) {
      auto current_ts = time(nullptr);
      if (current_ts > start_ts + 1 && rating_posted) {
        return current_ts + 60;
      }
      return current_ts;
    };
    create_rating(start_ts, time_function);
    rating->start();
    ASSERT_EQ(result.trading_results.size(), 0);

    auto deal_ts = time(nullptr);
    auto minute_ts = tr::get_minute_times(deal_ts);
    tr::minute_rating_uptr m_rating(
        new tr::minute_rating(minute_ts.first, minute_ts.second));
    m_rating->on_user_deal_won(deal_ts, 10, 10.0);
    m_rating->on_user_deal_won(deal_ts, 20, 5.2);
    m_rating->on_user_deal_won(deal_ts, 30, 0.01);
    m_rating->on_user_deal_won(deal_ts, 10, 20.0);
    rating->on_minute(std::move(m_rating));
    rating_posted = true;
    std::this_thread::sleep_for(std::chrono::seconds(3));
    ASSERT_EQ(result.trading_results.size(), 3);

    const tr::rating_result_t& user_10 = result.trading_results[10];
    ASSERT_EQ(user_10.user_id, 10);
    ASSERT_EQ(user_10.amount, 30.0);
    ASSERT_EQ(user_10.above_users.size(), 0);
    ASSERT_EQ(user_10.below_users.size(), 2);
    auto user_10_top_users_itr = user_10.top_users.begin();
    ASSERT_EQ(user_10_top_users_itr->first, 30.);
    ASSERT_TRUE(user_10_top_users_itr->second ==
                tr::rating_result_t::user_set_t{10});
    ASSERT_TRUE(user_10_top_users_itr->second ==
                tr::rating_result_t::user_set_t{10});
    ++user_10_top_users_itr;
    ASSERT_EQ(user_10_top_users_itr->first, 5.2);
    ASSERT_TRUE(user_10_top_users_itr->second ==
                tr::rating_result_t::user_set_t{20});
    ++user_10_top_users_itr;
    ASSERT_EQ(user_10_top_users_itr->first, 0.01);
    ASSERT_TRUE(user_10_top_users_itr->second ==
                tr::rating_result_t::user_set_t{30});

    const tr::rating_result_t& user_20 = result.trading_results[20];
    ASSERT_EQ(user_20.user_id, 20);
    ASSERT_EQ(user_20.amount, 5.2);
    ASSERT_EQ(user_20.above_users.size(), 1);
    ASSERT_EQ(user_20.below_users.size(), 1);

    const tr::rating_result_t& user_30 = result.trading_results[30];
    ASSERT_EQ(user_30.user_id, 30);
    ASSERT_EQ(user_30.amount, 0.01);
    ASSERT_EQ(user_30.above_users.size(), 2);
    ASSERT_EQ(user_30.below_users.size(), 0);

    rating->stop();
  }
  catch (std::exception& e) {
    FAIL() << e.what();
  }
}

struct ServiceFixture : public ::testing::Test {
  ServiceFixture() {}

  void create_service() { service_.reset(new tr::service(result.callback)); }

  void create_service(tr::time_function_t time_function) {
    start_ts = time(nullptr);
    start_minute_pair = tr::get_minute_times(start_ts);
    start_minute_ts = start_minute_pair.first;

    service_.reset(new tr::service(result.callback, time_function));
  }

  time_t start_ts, start_minute_ts;
  std::pair<time_t, time_t> start_minute_pair;
  tr::time_function_t time_function;

  test_get_rating_result result;
  std::unique_ptr<tr::service> service_;
};

TEST_F(ServiceFixture, StartStop) {
  try {
    create_service();
    service_->start();
    std::this_thread::sleep_for(std::chrono::seconds(1));
    service_->stop();
  }
  catch (std::exception& e) {
    FAIL() << e.what();
  }
}

TEST_F(ServiceFixture, OnUserRegistered) {
  try {
    create_service();
    service_->start();
    ASSERT_FALSE(service_->is_user_registered(100));
    ASSERT_FALSE(service_->is_user_registered(200));
    service_->on_user_registered(100, "user #100");
    service_->on_user_registered(200, "пользователь #200");
    std::this_thread::sleep_for(std::chrono::seconds(1));
    ASSERT_TRUE(service_->is_user_registered(100));
    ASSERT_TRUE(service_->is_user_registered(200));
    service_->stop();
  }
  catch (std::exception& e) {
    FAIL() << e.what();
  }
}

TEST_F(ServiceFixture, OnUserRenamed) {
  try {
    create_service();
    service_->start();
    ASSERT_FALSE(service_->is_user_registered(100));
    service_->on_user_registered(100, "user #100");
    std::this_thread::sleep_for(std::chrono::seconds(1));
    ASSERT_TRUE(service_->is_user_registered(100));
    service_->on_user_renamed(100, "пользователь #100");
    std::this_thread::sleep_for(std::chrono::seconds(1));
    ASSERT_TRUE(service_->is_user_registered(100));
    std::this_thread::sleep_for(std::chrono::seconds(1));
    service_->stop();
  }
  catch (std::exception& e) {
    FAIL() << e.what();
  }
}

TEST_F(ServiceFixture, OnUserConnectedDisconnected) {
  try {
    create_service();
    service_->start();
    auto user_id = 100;
    service_->on_user_registered(user_id, "user #100");
    std::this_thread::sleep_for(std::chrono::seconds(1));
    ASSERT_FALSE(service_->is_user_connected(user_id));
    service_->on_user_connected(user_id);
    std::this_thread::sleep_for(std::chrono::seconds(1));
    ASSERT_TRUE(service_->is_user_connected(user_id));
    service_->on_user_disconnected(user_id);
    std::this_thread::sleep_for(std::chrono::seconds(1));
    ASSERT_FALSE(service_->is_user_connected(user_id));
    service_->stop();
  }
  catch (std::exception& e) {
    FAIL() << e.what();
  }
}

TEST_F(ServiceFixture, OnUserDealWon1) {
  try {
    std::atomic_int minute_passed{0};
    time_function = [&](time_t*) {
      auto current_ts = time(nullptr);
      auto diff_ts = current_ts - start_ts;
      return start_minute_ts + 60 * minute_passed + diff_ts;
    };

    using namespace tr;
    create_service(time_function);
    service_->start();
    ASSERT_EQ(result.trading_results.size(), 0);
    user_id_t user_id = 100;
    service_->on_user_registered(user_id, "user #100");
    service_->on_user_registered(user_id + 1, "user #100");
    std::this_thread::sleep_for(std::chrono::seconds(1));
    ASSERT_TRUE(service_->is_user_registered(user_id));
    ASSERT_TRUE(service_->is_user_registered(user_id + 1));
    service_->on_user_connected(user_id);
    service_->on_user_connected(user_id + 1);
    std::this_thread::sleep_for(std::chrono::seconds(1));
    ASSERT_TRUE(service_->is_user_connected(user_id));

    service_->on_user_deal_won(time_function(nullptr), user_id, 100);
    service_->on_user_deal_won(time_function(nullptr), user_id, 200);
    service_->on_user_deal_won(time_function(nullptr), user_id, 300.5);
    ASSERT_TRUE(service_->is_user_connected(user_id));
    std::this_thread::sleep_for(std::chrono::seconds(1));
    minute_passed = 1;
    std::this_thread::sleep_for(std::chrono::seconds(1));
    ASSERT_EQ(result.trading_results.size(), 1);
    const auto& res = result.trading_results[100];
    ASSERT_EQ(res.user_id, 100);
    ASSERT_EQ(res.amount, 600.5);

    result.trading_results.clear();
    minute_passed = 2;
    std::this_thread::sleep_for(std::chrono::seconds(1));
    ASSERT_EQ(result.trading_results.size(), 1);
    const auto& res2 = result.trading_results[100];
    ASSERT_EQ(res2.user_id, 100);
    ASSERT_EQ(res2.amount, 600.5);

    result.trading_results.clear();
    service_->on_user_deal_won(time_function(nullptr), user_id, 400.5);
    service_->on_user_deal_won(time_function(nullptr), user_id + 1, 2000);
    std::this_thread::sleep_for(std::chrono::seconds(1));
    minute_passed = 3;
    std::this_thread::sleep_for(std::chrono::seconds(1));
    ASSERT_EQ(result.trading_results.size(), 2);
    const auto& res31 = result.trading_results[user_id];
    ASSERT_EQ(res31.user_id, user_id);
    ASSERT_EQ(res31.amount, 1001);
    ASSERT_EQ(res31.top_users.begin()->first, 2000);
    ASSERT_TRUE(res31.top_users.begin()->second ==
                rating_result_t::user_set_t{user_id + 1});

    const auto& res32 = result.trading_results[user_id + 1];
    ASSERT_EQ(res32.user_id, user_id + 1);
    ASSERT_EQ(res32.amount, 2000);
    ASSERT_TRUE(res32.top_users.begin()->second ==
                rating_result_t::user_set_t{user_id + 1});

    service_->stop();
  }
  catch (std::exception& e) {
    FAIL() << e.what();
  }
}
