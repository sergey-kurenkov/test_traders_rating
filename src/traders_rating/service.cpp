#include "traders_rating/service.h"
#include "traders_rating/utilities.h"

#include <functional>
#include <cassert>

namespace tr = ::traders_rating;

using unique_lock_t = std::unique_lock<std::mutex>;
using lock_guard_t = std::lock_guard<std::mutex>;

/*
 *
 */
tr::service::service(tr::upload_result_callback callback)
    : finish_thread_(false), upload_result_callback_(callback) {
  lock.clear();
  using namespace std::placeholders;
  user_registered_callback_ =
      std::bind(&service::process_user_registered, this, _1, _2);
  user_renamed_callback_ =
      std::bind(&service::process_user_renamed, this, _1, _2);
  user_connected_callback_ =
      std::bind(&service::process_user_connected, this, _1);
  user_disconnected_callback_ =
      std::bind(&service::process_user_disconnected, this, _1);
  user_deal_won_callback_ =
      std::bind(&service::process_user_deal_won, this, _1, _2, _3);
  get_connected_callback_ = std::bind(&service::get_connected_users, this, _1);
}

void tr::service::start() {
  finish_thread_ = false;
  th_ = std::thread(&tr::service::execute, this);
}

void tr::service::stop() {
  finish_thread_ = true;
  th_.join();
}

void tr::service::add_cmd(cmd_uptr cmd) {
  while (lock.test_and_set(std::memory_order_acquire))
    ;  // spin
  cmds_.push(std::move(cmd));
  lock.clear(std::memory_order_release);
}

std::pair<bool, tr::cmd_uptr> tr::service::get_cmd() {
  optional_cmd_t optional_cmd;
  while (lock.test_and_set(std::memory_order_acquire))
    ;  // spin
  if (cmds_.empty()) {
    optional_cmd.first = false;
  } else {
    optional_cmd.first = true;
    optional_cmd.second = std::move(cmds_.front());
    cmds_.pop();
  }
  lock.clear(std::memory_order_release);
  return optional_cmd;
}

void tr::service::on_user_registered(user_id_t id, const user_name_t& name) {
  add_cmd(cmd_uptr(new user_registered(id, name, user_registered_callback_)));
}

void tr::service::on_user_renamed(user_id_t id, const user_name_t& name) {
  add_cmd(cmd_uptr(new user_renamed(id, name, user_renamed_callback_)));
}

void tr::service::execute() {
  auto start_ts = time(nullptr);
  auto this_week_times = tr::get_week_times(start_ts);
  this_week_rating_ = week_rating_uptr(
      new week_rating(this_week_times.first, this_week_times.second,
                      get_connected_callback_, upload_result_callback_));
  this_week_rating_->start();

  auto this_minute_times = tr::get_minute_times(start_ts);
  this_minute_rating_ = minute_rating_uptr(
      new minute_rating(this_minute_times.first, this_minute_times.second));

  while (!finish_thread_) {
    auto current_ts = time(nullptr);

    if (current_ts >= this_minute_times.second) {
      this_week_rating_->on_minute(std::move(this_minute_rating_));
      this_minute_times = tr::get_minute_times(current_ts);
      this_minute_rating_ = minute_rating_uptr(
          new minute_rating(this_minute_times.first, this_minute_times.second));
    }

    if (current_ts >= this_week_times.second) {
      auto ts = this_week_rating_->start_ts();
      archive_week_ratings_.insert(
          std::make_pair(ts, std::move(this_week_rating_)));
      this_week_times = tr::get_week_times(current_ts);
      this_week_rating_ = week_rating_uptr(
          new week_rating(this_week_times.first, this_week_times.second,
                          get_connected_callback_, upload_result_callback_));
      this_week_rating_->start();
    }

    auto optional_cmd = get_cmd();
    if (!optional_cmd.first) {
      tr::yield_thread();
      continue;
    }

    optional_cmd.second->handle();
  }
  this_week_rating_->stop();
  for (auto& p : archive_week_ratings_) {
    p.second->stop();
  }
}

void tr::service::process_user_registered(user_id_t id,
                                          const user_name_t& name) {
  lock_guard_t lk(mt_);
  registered_users_.insert(std::make_pair(id, name));
}

void tr::service::process_user_renamed(user_id_t id, const user_name_t& name) {
  lock_guard_t lk(mt_);
  auto itr = registered_users_.find(id);
  if (itr != registered_users_.end()) {
    itr->second = name;
  }
}

void tr::service::process_user_connected(user_id_t id) {
  lock_guard_t lk(mt_);
  auto itr = registered_users_.find(id);
  if (itr != registered_users_.end()) {
    connected_users_.insert(id);
  }
}

void tr::service::process_user_disconnected(user_id_t id) {
  lock_guard_t lk(mt_);
  connected_users_.erase(id);
}

void tr::service::process_user_deal_won(time_t ts, user_id_t id, amount_t am) {
  this_minute_rating_->on_user_deal_won(ts, id, am);
}

void tr::service::get_connected_users(std::vector<user_id_t>& users) {
  unique_lock_t lk(mt_);
  auto connected_users_size = connected_users_.size();
  lk.unlock();
  users.reserve(connected_users_size + 10);
  lk.lock();
  for (auto user_id : connected_users_) {
    users.push_back(user_id);
  }
}

/*
 *
 */
tr::week_rating::week_rating(time_t start, time_t finish,
                             get_connected_callback get_connected,
                             upload_result_callback upload_result_callback_f)
    : week_rating(start, finish, get_connected, upload_result_callback_f,
                  &time) {}

tr::week_rating::week_rating(time_t start, time_t finish,
                             get_connected_callback get_connected,
                             upload_result_callback upload_result_callback_f,
                             time_function_t time_function)
    : start_ts_(start),
      finish_ts_(finish),
      finish_thread_(false),
      get_connected_callback_(get_connected),
      upload_result_callback_(upload_result_callback_f),
      time_function_(time_function),
      thread_started_(false),
      thread_finished_(false) {}

time_t tr::week_rating::start_ts() const { return start_ts_; }

bool tr::week_rating::started() const { return thread_started_; }

bool tr::week_rating::finished() const { return thread_finished_; }

void tr::week_rating::on_minute(tr::minute_rating_uptr minute_rating) {
  unique_lock_t lk(mt_);
  minute_ratings_.push(std::move(minute_rating));
  lk.unlock();
  cv_.notify_one();
}

void tr::week_rating::start() {
  finish_thread_ = false;
  th_ = std::thread(&week_rating::execute, this);
}

void tr::week_rating::stop() {
  unique_lock_t lk(mt_);
  finish_thread_ = true;
  cv_.notify_one();
  lk.unlock();
  th_.join();
}

void tr::week_rating::execute() {
  thread_started_ = true;
  struct on_finish_t {
    week_rating* p;
    ~on_finish_t() { p->thread_finished_ = true; }
  } on_finish{this};
  std::chrono::seconds wait_interval(1);
  auto current_minute = get_minute_times(time_function_(nullptr));
  // поток сам завершится через 5 секунду после окончания недели
  auto finish_thread_ts = finish_ts_ + 5;
  while (!finish_thread_) {
    auto current_ts = time_function_(nullptr);
    if (current_ts > finish_thread_ts) {
      finish_thread_ = true;
      continue;
    }

    minute_ratings_t copy_minute_ratings_;
    unique_lock_t lk(mt_);
    while (!minute_ratings_.empty()) {
      copy_minute_ratings_.push(std::move(minute_ratings_.front()));
      minute_ratings_.pop();
    }
    lk.unlock();

    while (!copy_minute_ratings_.empty()) {
      minute_rating& mr = *copy_minute_ratings_.front();
      if (mr.start_ts() >= start_ts_ && mr.finish_ts() <= finish_ts_) {
        update_week_rating(mr);
      }
      copy_minute_ratings_.pop();
    }

    // через 1 секунду после окончания минуты функция отправляет рейтинг
    if (current_ts < current_minute.second + 1) {
      lk.lock();
      if (finish_thread_) {
        continue;
      }
      cv_.wait_for(lk, wait_interval, [&]() { return !!finish_thread_; });
      continue;
    }

    send_rating();
    current_minute = get_minute_times(current_ts);
  }
}

void tr::week_rating::send_rating() {
  auto ts = time_function_(nullptr);
  std::vector<user_id_t> users;
  get_connected_callback_(users);
  for (auto user_id : users) {
    rating_result_t res;
    res.ts = ts;
    res.user_id = user_id;
    res.amount = 0;
    auto user_won_amount_itr = user_won_amount_.find(user_id);
    if (user_won_amount_itr != std::end(user_won_amount_)) {
      res.amount = user_won_amount_itr->second;
    } else {
      continue;
    }

    // top 10 users
    {
      rating_by_amount_t::iterator itr = rating_by_amount_.begin();
      for (auto i = 0U; i < 10 && i < rating_by_amount_.size(); ++i, ++itr) {
        auto user_pair = res.top_users.insert(
            std::make_pair(itr->first, rating_result_t::user_set_t()));
        rating_result_t::user_set_t& user_set = user_pair.first->second;
        for (auto k : itr->second) {
          user_set.insert(k);
        }
      }
    }

    // users above user_id
    rating_by_amount_t::iterator above_itr =
        rating_by_amount_.lower_bound(res.amount);
    if (above_itr != std::begin(rating_by_amount_)) {
      auto above_total = 0;
      do {
        --above_itr;
        auto user_pair = res.above_users.insert(
            std::make_pair(above_itr->first, rating_result_t::user_set_t()));
        rating_result_t::user_set_t& user_set = user_pair.first->second;
        for (auto k : above_itr->second) {
          user_set.insert(k);
        }
      } while (above_itr != std::begin(rating_by_amount_) &&
               above_total++ < 10);
    }

    // users below user_id
    {
      rating_by_amount_t::iterator below_itr =
          rating_by_amount_.upper_bound(res.amount);
      auto below_total = 0;
      while (below_itr != rating_by_amount_.end() && below_total < 10) {
        auto user_pair = res.below_users.insert(
            std::make_pair(below_itr->first, rating_result_t::user_set_t()));
        rating_result_t::user_set_t& user_set = user_pair.first->second;
        for (auto k : below_itr->second) {
          user_set.insert(k);
        }
        ++below_itr;
      }
    }

    upload_result_callback_(res);
  }
}

void tr::week_rating::update_week_rating(const tr::minute_rating& mr) {
  for (const auto& user_data : mr) {
    user_id_t user_id = user_data.first;
    amount_t amount = user_data.second;
    auto itr = user_won_amount_.find(user_id);
    amount_t prev_amount;
    if (itr == std::end(user_won_amount_)) {
      std::tie(itr, std::ignore) =
          user_won_amount_.insert(std::make_pair(user_id, amount));
      prev_amount = 0;
    } else {
      prev_amount = itr->second;
      itr->second += amount;
    }
    if (prev_amount > 0) {
      auto ra_itr = rating_by_amount_.find(prev_amount);
      assert(ra_itr != rating_by_amount_.end());
      same_amount_users_t& same_amount_users = ra_itr->second;
      same_amount_users.erase(user_id);
    }
    amount_t new_amount = itr->second;
    auto new_ra_itr = rating_by_amount_.find(new_amount);
    if (new_ra_itr == std::end(rating_by_amount_)) {
      std::tie(new_ra_itr, std::ignore) = rating_by_amount_.insert(
          std::make_pair(new_amount, same_amount_users_t{}));
    }
    new_ra_itr->second.insert(user_id);
  }
}

/*
 *
 */
tr::minute_rating::minute_rating(time_t start, time_t finish)
    : start_ts_(start), finish_ts_(finish) {}

void tr::minute_rating::on_user_deal_won(time_t ts, user_id_t id, amount_t am) {
  if (ts >= start_ts_ && ts < finish_ts_) {
    auto itr = user_won_amount_.find(id);
    if (itr != user_won_amount_.end()) {
      itr->second += am;
    } else {
      user_won_amount_.insert(std::make_pair(id, am));
    }
  }
}

time_t tr::minute_rating::start_ts() const { return start_ts_; }

time_t tr::minute_rating::finish_ts() const { return finish_ts_; }
