#ifndef traders_rating_service_h
#define traders_rating_service_h

namespace traders_rating {

struct service_cmd {
    virtual ~service_cmd() = 0;
    virtual void run() = 0;
};

struct user_registered : public service_cmd {};

struct user_connected : public service_cmd {};

class service {
 public:
  service();
  void on_user_registered();
  void on_user_connected();

 private:
    void execute();
};
}

#endif  // traders_rating_service_h
