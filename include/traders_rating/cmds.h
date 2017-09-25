#ifndef traders_rating_cmds_h
#define traders_rating_cmds_h

#include <string>
#include <memory>
#include <functional>

namespace traders_rating {

using user_id_t = uint64_t;
using amount_t = double;
using user_name_t = std::string;

/*
 *
 */
class cmd {
 public:
  virtual ~cmd() {};
  virtual void handle() = 0;
};
using cmd_uptr = std::unique_ptr<cmd>;

/*
 *
 */
using user_registered_callback =
    std::function<void(user_id_t, const user_name_t&)>;

class user_registered : public cmd {
 public:
  user_registered(user_id_t, const user_name_t&, user_registered_callback);

 private:
  user_id_t id;
  user_name_t user_name;
  user_registered_callback callback_;

 private:
  void handle() override;
};

/*
 *
 */
using user_renamed_callback =
    std::function<void(user_id_t, const user_name_t&)>;

class user_renamed : public cmd {
 public:
  user_renamed(user_id_t, const user_name_t&, user_renamed_callback);

 private:
  user_id_t id;
  user_name_t user_name;
  user_renamed_callback callback_;

 private:
  void handle() override;
};

/*
 *
 */
using user_connected_callback = std::function<void(user_id_t)>;

class user_connected : public cmd {
 public:
  user_connected(user_id_t, user_connected_callback);

 private:
  user_id_t id;
  user_connected_callback callback_;

 private:
  void handle() override;
};

/*
 *
 */
using user_disconnected_callback = std::function<void(user_id_t)>;

class user_disconnected : public cmd {
 public:
  user_disconnected(user_id_t, user_disconnected_callback);

 private:
  user_id_t id;
  user_disconnected_callback callback_;

 private:
  void handle() override;
};

/*
 *
 */
using user_deal_won_callback = std::function<void(time_t, user_id_t, amount_t)>;

class user_deal_won : public cmd {
 public:
  user_deal_won(time_t, user_id_t, amount_t, user_deal_won_callback);

 private:
  time_t ts;
  user_id_t id;
  amount_t amount;
  user_deal_won_callback callback_;

 private:
  void handle() override;
};

}  // namespace traders_rating

#endif  // traders_rating_cmds_h
