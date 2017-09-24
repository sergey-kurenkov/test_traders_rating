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
tr::service::service() : finish_thread_(false) {
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
  finish_thread_ == false;
  th_ = std::thread(&tr::service::execute, this);
}

void tr::service::stop() {
  finish_thread_ == true;
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
  this_week_rating_ = week_rating_uptr(new week_rating(
      this_week_times.first, this_week_times.second, get_connected_callback_));
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
                          get_connected_callback_));
      this_week_rating_->start();
    }

    auto optional_cmd = get_cmd();
    if (!optional_cmd.first) {
      std::this_thread::yield();
      continue;
    }

    optional_cmd.second->handle();
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
  users.reserve(connected_users_size+10);
  lk.lock();
  for (auto user_id : connected_users_) {
    users.push_back(user_id);
  }
}

/*
 *
 */
tr::week_rating::week_rating(time_t start, time_t finish,
                             get_connected_callback get_connected)
    : start_ts_(start),
      finish_ts_(finish),
      finish_thread_(false),
      get_connected_callback_(get_connected) {}

time_t tr::week_rating::start_ts() const { return start_ts_; }

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
  lk.unlock();
  cv_.notify_one();
  th_.join();
}

void tr::week_rating::execute() {
  std::chrono::seconds wait_interval(1);
  auto current_minute = get_minute_times(time(nullptr));
  bool current_minute_report_done = false;
  // поток сам завершится через 1 минуту после окончания недели
  auto finish_thread_ts = finish_ts_ + 60;
  while (!finish_thread_) {
    auto current_ts = time(nullptr);
    if (current_ts > finish_thread_ts) {
      finish_thread_ = true;
      continue;
    }

    unique_lock_t lk(mt_);
    if (minute_ratings_.empty()) {
      // через 1 секунду после окончания минуты функция отправляет рейтинг
      if (current_minute_report_done ||
          current_ts - 1 < current_minute.second) {
        cv_.wait_for(lk, wait_interval);
        continue;
      }
      lk.unlock();

      send_rating();
      current_minute = get_minute_times(current_ts);
      current_minute_report_done = false;
    }

    minute_rating_uptr mr_ptr = std::move(minute_ratings_.front());
    minute_ratings_.pop();
    lk.unlock();
    if (mr_ptr->start_ts() >= start_ts_ && mr_ptr->finish_ts() <= finish_ts_) {
      update_week_rating(*mr_ptr);
    }
    if (!current_minute_report_done && current_ts - 1 > current_minute.second) {
      send_rating();
      current_minute = get_minute_times(current_ts);
      current_minute_report_done = false;
    }
  }
}

void tr::week_rating::send_rating() {
  std::vector<user_id_t> users;
  get_connected_callback_(users);
  for (auto user_id : users) {
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

tr::minute_rating::iterator tr::minute_rating::begin() const {
  return tr::minute_rating::iterator(user_won_amount_.begin());
}

tr::minute_rating::iterator tr::minute_rating::end() const {
  return tr::minute_rating::iterator(user_won_amount_.end());
}

/*
 *
 */
tr::minute_rating::iterator::iterator(
    tr::minute_rating::user_won_amount_t::const_iterator itr)
    : itr_(itr) {}

const tr::minute_rating::value_type tr::minute_rating::iterator::operator*()
    const {
  return *itr_;
}

bool tr::minute_rating::iterator::operator!=(tr::minute_rating::iterator other)
    const {
  return itr_ != other.itr_;
}

tr::minute_rating::iterator& tr::minute_rating::iterator::operator++() {
  ++itr_;
  return *this;
}

time_t tr::minute_rating::start_ts() const { return start_ts_; }

time_t tr::minute_rating::finish_ts() const { return finish_ts_; }
