#ifndef traders_rating_service_h
#define traders_rating_service_h

#include <unordered_map>
#include <unordered_set>
#include <string>
#include <map>
#include <set>
#include <queue>
#include <memory>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <thread>
#include <iterator>

#include "traders_rating/cmds.h"

namespace traders_rating {

/*
 *
 */
class minute_rating {
 private:
  using user_won_amount_t = std::unordered_map<user_id_t, amount_t>;
  using value_type = std::pair<user_id_t, amount_t>;

 public:
  class iterator : public std::iterator<std::input_iterator_tag, value_type> {
   public:
    explicit iterator(user_won_amount_t::const_iterator itr) : itr_(itr) {}
    const value_type operator*() const { return *itr_; }
    bool operator!=(iterator other) const { return itr_ != other.itr_; }
    iterator& operator++() {
      ++itr_;
      return *this;
    }

   private:
    user_won_amount_t::const_iterator itr_;
  };

 public:
  minute_rating(time_t start, time_t finish);
  void on_user_deal_won(time_t, user_id_t, amount_t);
  time_t start_ts() const;
  time_t finish_ts() const;
  iterator begin() const { return iterator(user_won_amount_.begin()); }
  iterator end() const { return iterator(user_won_amount_.end()); }

 private:
  time_t start_ts_;
  time_t finish_ts_;
  user_won_amount_t user_won_amount_;
};
using minute_rating_uptr = std::unique_ptr<minute_rating>;

struct rating_result_t {
  using user_set_t = std::set<user_id_t>;
  using rating_t = std::map<amount_t, user_set_t, std::greater<amount_t>>;

  time_t ts;
  user_id_t user_id;
  amount_t amount;

  rating_t top_users;
  rating_t above_users;
  rating_t below_users;
};

using upload_result_callback = std::function<void(const rating_result_t&)>;

/*
 *
 */
using get_connected_callback = std::function<void(std::vector<user_id_t>&)>;
using time_function_t = std::function<time_t(time_t*)>;

class week_rating {
 public:
  week_rating(time_t start, time_t finish, get_connected_callback,
              upload_result_callback);
  week_rating(time_t start, time_t finish, get_connected_callback,
              upload_result_callback, time_function_t);
  void start();
  void stop();
  void on_minute(minute_rating_uptr);
  time_t start_ts() const;
  bool started() const;
  bool finished() const;

 private:
  void execute();

 private:
  using same_amount_users_t = std::unordered_set<user_id_t>;
  using rating_by_amount_t =
      std::map<amount_t, same_amount_users_t, std::greater<amount_t>>;
  using minute_ratings_t = std::queue<minute_rating_uptr>;
  using user_won_amount_t = std::unordered_map<user_id_t, amount_t>;

 private:
  time_t start_ts_;
  time_t finish_ts_;
  std::mutex mt_;
  std::condition_variable cv_;
  std::thread th_;
  std::atomic_bool finish_thread_;
  minute_ratings_t minute_ratings_;
  rating_by_amount_t rating_by_amount_;
  user_won_amount_t user_won_amount_;
  get_connected_callback get_connected_callback_;
  upload_result_callback upload_result_callback_;
  time_function_t time_function_;
  std::atomic_bool thread_started_;
  std::atomic_bool thread_finished_;

 private:
  void update_week_rating(const minute_rating& mr);
  void send_rating();
};

using week_rating_uptr = std::unique_ptr<week_rating>;

/*
 *
 */
class service {
 public:
  service(upload_result_callback);
  void start();
  void stop();

  void on_user_registered(user_id_t, const user_name_t&);
  void on_user_renamed(user_id_t, const user_name_t&);
  void on_user_connected(user_id_t);
  void on_user_disconnected(user_id_t);
  void on_user_deal_won(time_t, user_id_t, amount_t);

 private:
  using optional_cmd_t = std::pair<bool, cmd_uptr>;
  using archive_week_ratings_t = std::map<time_t, week_rating_uptr>;
  using registered_users_t = std::unordered_map<user_id_t, user_name_t>;
  using connected_users_t = std::unordered_set<user_id_t>;

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

  user_registered_callback user_registered_callback_;
  user_renamed_callback user_renamed_callback_;
  user_connected_callback user_connected_callback_;
  user_disconnected_callback user_disconnected_callback_;
  user_deal_won_callback user_deal_won_callback_;
  get_connected_callback get_connected_callback_;

  registered_users_t registered_users_;
  connected_users_t connected_users_;

  upload_result_callback upload_result_callback_;

 private:
  void execute();
  void add_cmd(cmd_uptr);
  optional_cmd_t get_cmd();
  void process_user_registered(user_id_t, const user_name_t&);
  void process_user_renamed(user_id_t, const user_name_t&);
  void process_user_connected(user_id_t);
  void process_user_disconnected(user_id_t);
  void process_user_deal_won(time_t, user_id_t, amount_t);
  void get_connected_users(std::vector<user_id_t>&);
};
}

#endif  // traders_rating_service_h
