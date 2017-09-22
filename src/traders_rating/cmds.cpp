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
                                     user_registered_handler handler)
    : id(id), user_name(user_name), handler(handler) {}

void tr::user_registered::handle() { handler(id, user_name); }

/*
 *
 */
tr::user_renamed::user_renamed(user_id_t id, const user_name_t& user_name,
                               user_renamed_handler handler)
    : id(id), user_name(user_name), handler(handler) {}

void tr::user_renamed::handle() { handler(id, user_name); }

/*
 *
 */
tr::user_connected::user_connected(user_id_t id, user_connected_handler handler)
    : id(id), handler(handler) {}

void tr::user_connected::handle() { handler(id); }

/*
 *
 */
tr::user_disconnected::user_disconnected(user_id_t id,
                                         user_disconnected_handler handler)
    : id(id), handler(handler) {}

void tr::user_disconnected::handle() { handler(id); }

/*
 *
 */
tr::user_deal_won::user_deal_won(time_t ts, user_id_t id, amount_t amount,
                                 user_deal_won_handler handler)
    : ts(ts), id(id), amount(amount), handler(handler) {}

void tr::user_deal_won::handle() { handler(ts, id, amount); }
