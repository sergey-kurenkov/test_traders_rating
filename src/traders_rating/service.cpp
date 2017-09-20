#include "traders_rating/service.h"

namespace tr = ::traders_rating;

/*
 *
 */

/*
 *
 */
tr::service::service() : finish_(false) {
}

/*
 *
 */
void tr::service::start() {

}

/*
 *
 */
void tr::service::stop() {
  finish_ = true;
}

void tr::service::add_cmd(cmd_uptr cmd) {
  while (lock.test_and_set(std::memory_order_acquire))
    ; // spin
  cmds_.push(std::move(cmd));
  lock.clear(std::memory_order_release);
}

/*
 *
 */
void tr::service::on_user_registered(user_id_t id, const user_name_t& name) {
  add_cmd(cmd_uptr(new user_registered(id, name)));
}

/*
 *
 */
void tr::service::execute() {
  while (!finish_) {

  }
}


