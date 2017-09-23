#include "gtest/gtest.h"

#include "traders_rating/cmds.h"
#include "traders_rating/service.h"

/*
 *
 */
namespace tr = ::traders_rating;

struct user_registered_test : public ::testing::Test {
	user_registered_test() {
		using namespace std::placeholders;
		test_callback_ = std::bind(&user_registered_test::test_callback, this, _1, _2);
	}

	void test_callback(user_id_t, user_name_t) {

	}

	user_registered_callback test_callback_;
};

TEST(user_registered_test, constuctor) {
	cmd = tr::user_registered_test(1, "test", test_callback_);
}