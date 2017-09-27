#include <benchmark/benchmark.h>

#include "traders_rating/service.h"
#include "traders_rating/cmds.h"
#include "traders_rating/utilities.h"

#include <iostream>

namespace tr = ::traders_rating;

static void BM_MinuteRatingInsert(benchmark::State& state) {
	time_t ts = time(nullptr);
	auto minute_times = tr::get_minute_times(ts);
	tr::minute_rating rating(minute_times.first, minute_times.second);
	tr::user_id_t user_id = 0;
	tr::user_id_t total_users = 1000000;
	while (state.KeepRunning()) {
		rating.on_user_deal_won(ts, ++user_id % total_users, 1.);
	}
}

BENCHMARK(BM_MinuteRatingInsert);

struct get_rating_result_t {
  get_rating_result_t()
      : callback(std::bind(&get_rating_result_t::upload, this,
                           std::placeholders::_1)) {}
  void upload(const tr::rating_result_t& trading_result) {
  }
  tr::upload_result_callback callback;
};



static void BM_ServiceOnUserDealWon(benchmark::State& state) {
    get_rating_result_t get_rating_result; 
    tr::service service(get_rating_result.callback);
    tr::user_id_t user_id = 0;
    tr::user_id_t total_users = 1000000;
    service.start();
    for (tr::user_id_t i = 1; i < total_users; ++i) {
        service.on_user_registered(++i, "user");
        service.on_user_connected(i);
    }
    auto ts = time(nullptr);
    uint_fast64_t send_cmds = 0;
    auto on_start_processed_cmds = service.processed_cmds();
    while (state.KeepRunning()) {
        service.on_user_deal_won(ts, ++user_id % total_users, 1.);
        ++send_cmds;
    }
    std::cout << "send_cmds: " << send_cmds
            << ", processed commands: "<< (service.processed_cmds() - on_start_processed_cmds) << std::endl;
    service.stop();
}

BENCHMARK(BM_ServiceOnUserDealWon);


BENCHMARK_MAIN();
