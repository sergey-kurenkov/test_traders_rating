#include <benchmark/benchmark.h>

#include "traders_rating/service.h"
#include "traders_rating/cmds.h"
#include "traders_rating/utilities.h"

#include <iostream>

namespace tr = ::traders_rating;

const tr::user_id_t MAX_TEST_USER_ID = 1000000;

static void BM_MinuteRatingInsert(benchmark::State& state) {
  time_t ts = time(nullptr);
  auto minute_times = tr::get_minute_times(ts);
  tr::minute_rating rating(minute_times.first, minute_times.second);
  tr::user_id_t user_id = 0;
  tr::user_id_t total_users = MAX_TEST_USER_ID;
  while (state.KeepRunning()) {
    rating.on_user_deal_won(ts, ++user_id % total_users, 1.);
  }
}

BENCHMARK(BM_MinuteRatingInsert);

struct get_rating_result_t {
  get_rating_result_t()
      : upload_callback(std::bind(&get_rating_result_t::upload, this,
                                  std::placeholders::_1)) {}
  void upload(const tr::rating_result_t& trading_result) {}
  tr::upload_result_callback upload_callback;
};

struct OnUserRegisteredFixture : public benchmark::Fixture,
                                 get_rating_result_t {
  void SetUp(const benchmark::State& state) {
    if (state.thread_index == 0) {
      service_uptr.reset(new tr::service(upload_callback));
      total_users = state.range(0);
      service_uptr->start();
      on_start_processed_cmds = service_uptr->processed_cmds();
    }
  }

  void TearDown(const benchmark::State& state) {
    if (state.thread_index == 0) {
      service_uptr->stop();
    }
  }

  tr::user_id_t total_users;
  std::unique_ptr<tr::service> service_uptr;
  time_t ts;
  uint64_t on_start_processed_cmds = 0;
};

BENCHMARK_DEFINE_F(OnUserRegisteredFixture, Test)(benchmark::State& state) {
  std::srand(std::time(0));
  const std::string test_name("user");
  while (state.KeepRunning()) {
    service_uptr->on_user_registered(std::rand() % total_users, test_name);
  }
}

BENCHMARK_REGISTER_F(OnUserRegisteredFixture, Test)->Range(1, MAX_TEST_USER_ID);

BENCHMARK_REGISTER_F(OnUserRegisteredFixture, Test)
    ->Arg(MAX_TEST_USER_ID)
    ->Threads(1)
    ->Threads(2);

struct OnUserConnectedFixture : public benchmark::Fixture, get_rating_result_t {
  void SetUp(const benchmark::State& state) {
    if (state.thread_index == 0) {
      service_uptr.reset(new tr::service(upload_callback));
      total_users = state.range(0);
      service_uptr->start();
      auto on_start_processed_cmds = service_uptr->processed_cmds();
      for (tr::user_id_t i = 1; i < total_users; ++i) {
        service_uptr->on_user_registered(++i, test_name);
      }
      on_start_processed_cmds = service_uptr->processed_cmds();
    }
  }

  void TearDown(const benchmark::State& state) {
    if (state.thread_index == 0) {
      service_uptr->stop();
    }
  }

  tr::user_id_t total_users;
  const std::string test_name = "user";
  std::unique_ptr<tr::service> service_uptr;
  uint64_t on_start_processed_cmds = 0;
};

BENCHMARK_DEFINE_F(OnUserConnectedFixture, Test)(benchmark::State& state) {
  std::srand(std::time(0));
  while (state.KeepRunning()) {
    service_uptr->on_user_connected(std::rand() % total_users);
  }
}

BENCHMARK_REGISTER_F(OnUserConnectedFixture, Test)->Range(1, MAX_TEST_USER_ID);

BENCHMARK_REGISTER_F(OnUserConnectedFixture, Test)
    ->Arg(MAX_TEST_USER_ID)
    ->Threads(1)
    ->Threads(2);

struct OnUserDealWonFixture : public benchmark::Fixture, get_rating_result_t {
  void SetUp(const benchmark::State& state) {
    if (state.thread_index == 0) {
      service_uptr.reset(new tr::service(upload_callback));
      service_uptr->start();
      total_users = state.range(0);
      const std::string test_name("user");
      for (tr::user_id_t i = 1; i < total_users; ++i) {
        service_uptr->on_user_registered(++i, test_name);
        service_uptr->on_user_connected(i);
      }
      on_start_processed_cmds = service_uptr->processed_cmds();
    }
  }

  void TearDown(const benchmark::State& state) {
    if (state.thread_index == 0) {
      service_uptr->stop();
    }
  }

  tr::user_id_t total_users;
  std::unique_ptr<tr::service> service_uptr;
  uint64_t on_start_processed_cmds = 0;
};

BENCHMARK_DEFINE_F(OnUserDealWonFixture, Test)(benchmark::State& state) {
  std::srand(std::time(0));
  while (state.KeepRunning()) {
    service_uptr->on_user_deal_won(time(nullptr), std::rand() % total_users,
                                   1.);
  }
}

BENCHMARK_REGISTER_F(OnUserDealWonFixture, Test)->Range(1, MAX_TEST_USER_ID);

BENCHMARK_REGISTER_F(OnUserDealWonFixture, Test)
    ->Arg(MAX_TEST_USER_ID)
    ->Threads(1)
    ->Threads(2);

BENCHMARK_MAIN();
