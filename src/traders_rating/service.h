#ifndef traders_rating_service_h
#define traders_rating_service_h

#include <unordered_map>
#include <unordered_set>
#include <string>
#include <map>
#include <queue>
#include <memory>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <thread>

#include "traders_rating/cmds.h"

namespace traders_rating {

/*
 *
 */
using user_won_amount_t = std::unordered_map<user_id_t, amount_t>;
using connected_users_t = std::unordered_set<user_id_t>;


/*
 *
 */
class minute_rating {
 public:
  minute_rating(time_t start, time_t finish);
  bool is_in_interval(time_t) const;
  void on_user_deal_won(user_id_t, amount_t, time_t);

 private:
  time_t start_;
  time_t finish_;
  user_won_amount_t user_won_amount_;
};
using minute_rating_uptr = std::unique_ptr<minute_rating>;


/*
 *
 */
class week_rating {
 public:
  week_rating(time_t start, time_t finish);
  void start();
  void stop();
  void on_minute(minute_rating_uptr);
  void on_user_connected(user_id_t);
  void on_user_disconnected(user_id_t);
  time_t start_ts() const;

 private:
  void execute();

 private:
  using same_amount_users_t = std::unordered_set<user_id_t>;
  using rating_by_amount_t = std::map<amount_t, same_amount_users_t>;
  using minute_ratings_t = std::queue<minute_rating_uptr>;

 private:
  time_t start_;
  time_t finish_;
  std::mutex mt_;
  std::condition_variable cv_;
  std::thread th_;
  std::atomic_bool finish_thread_;
  minute_ratings_t minute_ratings_;
  connected_users_t connected_users_;
  rating_by_amount_t rating_by_amount_;
  user_won_amount_t user_won_amount_;
};

using week_rating_uptr = std::unique_ptr<week_rating>;


/*
 *
 */
class service {
 public:
  service();
  void start();
  void stop();

  void on_user_registered(user_id_t, const user_name_t&);
  void on_user_renamed(user_id_t, user_name_t);
  void on_user_connected(user_id_t);
  void on_user_disconnected(user_id_t);
  void on_user_deal_won(time_t, user_id_t, amount_t);

 private:
  using optional_cmd_t = std::pair<bool, cmd_uptr>;
  using archive_week_ratings_t = std::map<time_t, week_rating_uptr>;


 private:
  std::atomic_flag lock;
  std::queue<cmd_uptr> cmds_;
  std::thread th_;
  std::atomic_bool finish_thread_;
  std::mutex mt_;
  std::condition_variable cv_;
  week_rating_uptr this_week_rating_;
  minute_rating_uptr this_minute_rating_;
  archive_week_ratings_t archive_week_ratings_;
  user_registered_handler user_registered_handler_;
  user_deal_won_handler user_deal_won_handler_;

 private:
  void execute();
  void add_cmd(cmd_uptr);
  optional_cmd_t get_cmd();
  void process_user_registered(user_id_t, const user_name_t&);
  void process_user_deal_won(time_t, user_id_t, amount_t);
};
}

#endif  // traders_rating_service_h
