#include "traders_rating/service.h"
#include "tr_utilities/get_time.h"

#include <functional>

namespace tr = ::traders_rating;

using unique_lock_t = std::unique_lock<std::mutex>;
using lock_guard_t = std::lock_guard<std::mutex>;

/*
 *
 */
tr::service::service()
  : finish_thread_(false) {
  using namespace std::placeholders;
  user_registered_handler_ = std::bind(&service::process_user_registered,
                                       this, _1, _2);
  user_deal_won_handler_ = std::bind(&service::process_user_deal_won,
                                     this, _1, _2, _3);
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
    ; // spin
  cmds_.push(std::move(cmd));
  lock.clear(std::memory_order_release);
}


std::pair<bool, tr::cmd_uptr> tr::service::get_cmd() {
  optional_cmd_t optional_cmd;
  while (lock.test_and_set(std::memory_order_acquire))
    ; // spin
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
  add_cmd(cmd_uptr(new user_registered(id, name, user_registered_handler_)));
}


void tr::service::execute() {
  auto start_ts = time(nullptr);
  auto this_week_times = tr::get_week_times(start_ts);
  this_week_rating_ = week_rating_uptr(
      new week_rating(this_week_times.first,this_week_times.second));
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
          new week_rating(this_week_times.first,this_week_times.second));
      this_week_rating_->start();
    }

    auto optional_cmd = get_cmd();
    if (!optional_cmd.first) {
      continue;
    }

    optional_cmd.second->handle();
  }
}


void tr::service::process_user_registered(user_id_t, const user_name_t&) {

}


void tr::service::process_user_deal_won(time_t, user_id_t, amount_t) {

}


/*
 *
 */
tr::week_rating::week_rating(time_t start, time_t finish)
  : start_(start), finish_(finish), finish_thread_(false) {
}


time_t tr::week_rating::start_ts() const {
  return start_;
}


void tr::week_rating::on_minute(tr::minute_rating_uptr minute_rating) {
  unique_lock_t lk(mt_);
  minute_ratings_.push(std::move(minute_rating));
  lk.unlock();
  cv_.notify_one();
}


void tr::week_rating::start() {
  th_ = std::thread(&week_rating::execute, this);
}


void tr::week_rating::stop() {
  finish_thread_ = true;
  th_.join();
}


void tr::week_rating::execute() {
  while (!finish_thread_) {

  }
}


/*
 *
 */
tr::minute_rating::minute_rating(time_t start, time_t finish)
  : start_(start), finish_(finish) {
}
