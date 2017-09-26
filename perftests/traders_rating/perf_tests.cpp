#include <benchmark/benchmark.h>

#include "traders_rating/service.h"
#include "traders_rating/cmds.h"
#include "traders_rating/utilities.h"

using namespace tr = ::traders_rating;

static void BM_MinuteRatingInsert(benchmark::State& state) {
	time_t ts = time(nullptr);
	auto minute_times = tr::get_minute_times(ts)
	tr::minute_rating rating(minute_times.first, minute_times.second);
	tr::user_id_t user_id = 0;
	tr::user_id_t total_users = 1000000;
	while (state.KeepRunning()) {
		rating.on_user_deal(ts, ++user_id % total_users, 1.);
	}
}

BENCHMARK(BM_MinuteRatingInsert);

BENCHMARK_MAIN();
