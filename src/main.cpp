#include "traders_rating/service.h"

void upload_trading_results(const traders_rating::rating_result_t&) {}

int main() {
  traders_rating::service srv(&upload_trading_results);
  srv.start();
  srv.stop();
}
