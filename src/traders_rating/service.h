#ifndef traders_rating_service_h
#define traders_rating_service_h

#include <unordered_map>
#include <string>
#include <multimap>
#include <boost/circular_buffer.hpp>

namespace traders_rating {

using user_id_t = uint64_t;
using amount_t = double;

enum class cmd_type_t {
    user_registered,
    user_renamed,
    user_connected,
    user_disconnected,
    user_deal_won
};

struct user_cmd_t {
    user_id_t user_id;
    cmd_type_t cmd_type;
    std::string user_name;
};

struct deal_cmd_t {
    user_id_t user_id;
    amount_t amount;
};

class week_rating {
 public:
  week_rating(time_t start, time_t finish);
  void on_user_deal_won();
  void on_user_connected();
  void on_user_disconnected();
  void on_send_rating();

 private:
  time_t start_;
  time_t finish_;
  using users_t = std::std::unordered_map<user_id_t, amount_t>
  using connected_users_t = std::std::unordered_set<user_id_t>
  using same_amount_users_t = std::unordered_set<user_id_t>;
  using rating_by_amount_t = std::map<amount_t, same_amount_users_t>;
  users_t users_;
  connected_users_t connected_users_;
  rating_by_amount_t rating_by_amount_;
};

class service {
 public:
  service();
  void start();
  void stop();

  void on_user_registered();
  void on_user_renamed();
  void on_user_connected();
  void on_user_disconnected();
  void on_user_deal_won();

 private:
  void execute();

 private:
  time_t this_week_start_;
  time_t this_week_finish_;
  week_rating this_week_rating_;
};

}

#endif  // traders_rating_service_h
