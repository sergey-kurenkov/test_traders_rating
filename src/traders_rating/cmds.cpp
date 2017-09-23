#include "traders_rating/cmds.h"
#include "traders_rating/service.h"

/*
 *
 */
namespace tr = ::traders_rating;

/*
 *
 */
tr::user_registered::user_registered(user_id_t id, const user_name_t& user_name,
                                     user_registered_callback callback)
    : id(id), user_name(user_name), callback_(callback) {}

void tr::user_registered::handle() { callback_(id, user_name); }

/*
 *
 */
tr::user_renamed::user_renamed(user_id_t id, const user_name_t& user_name,
                               user_renamed_callback callback)
    : id(id), user_name(user_name), callback_(callback) {}

void tr::user_renamed::handle() { callback_(id, user_name); }

/*
 *
 */
tr::user_connected::user_connected(user_id_t id, user_connected_callback callback)
    : id(id), callback_(callback) {}

void tr::user_connected::handle() { callback_(id); }

/*
 *
 */
tr::user_disconnected::user_disconnected(user_id_t id,
                                         user_disconnected_callback callback)
    : id(id), callback_(callback) {}

void tr::user_disconnected::handle() { callback_(id); }

/*
 *
 */
tr::user_deal_won::user_deal_won(time_t ts, user_id_t id, amount_t amount,
                                 user_deal_won_callback callback)
    : ts(ts), id(id), amount(amount), callback_(callback) {}

void tr::user_deal_won::handle() { callback_(ts, id, amount); }
