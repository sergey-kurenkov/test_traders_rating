#ifndef traders_rating_service_h
#define traders_rating_service_h

#include <unordered_map>
#include <unordered_set>
#include <string>
#include <map>
#include <queue>
#include <memory>
#include <mutex>
#include <atomic>
#include <thread>

namespace traders_rating {

using user_id_t = uint64_t;
using amount_t = double;
using user_name_t = std::string;
using user_won_amount_t = std::unordered_map<user_id_t, amount_t>;
using connected_users_t = std::unordered_set<user_id_t>;

/*
 *
 */
class cmd {
 public:
  virtual ~cmd() {};
  virtual void handle(class service&) = 0;
};
using cmd_uptr = std::unique_ptr<cmd>;

/*
 *
 */
class user_registered : public cmd {
 public:
  user_registered(user_id_t, const user_name_t&);
  user_id_t id;
  user_name_t user_name;
  void handle(service&) override;
};

/*
 *
 */
struct user_deal_won : public cmd {
  user_id_t id;
  amount_t amount;
  void handle(service&) override;
};

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
  void on_minute_rating(minute_rating_uptr);
  void on_user_connected(user_id_t);
  void on_user_disconnected(user_id_t);

 private:
  void execute();

 private:
  using same_amount_users_t = std::unordered_set<user_id_t>;
  using rating_by_amount_t = std::map<amount_t, same_amount_users_t>;

  std::mutex mt_;
  connected_users_t connected_users_;

  rating_by_amount_t rating_by_amount_;
  user_won_amount_t user_won_amount_;

  using minute_ratings_t = std::queue<minute_rating_uptr>;
  minute_ratings_t minute_ratings_;
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
  void execute();
  void add_cmd(cmd_uptr);

 private:
  minute_rating_uptr this_minute_rating_;
  week_rating_uptr this_week_rating_;

  std::atomic_flag lock;
  std::queue<cmd_uptr> cmds_;

  std::thread th_;
  std::atomic_bool finish_;

};
}

#endif  // traders_rating_service_h
