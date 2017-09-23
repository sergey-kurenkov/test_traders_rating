#include "gtest/gtest.h"

#include "traders_rating/cmds.h"
#include "traders_rating/service.h"

/*
 *
 */
namespace tr = ::traders_rating;

struct UserRegisteredTest : public ::testing::Test {
	UserRegisteredTest() {
		using namespace std::placeholders;
		test_callback_ = std::bind(&UserRegisteredTest::test_callback, this, _1, _2);
	}

	void test_callback(tr::user_id_t, tr::user_name_t) {

	}

	tr::user_registered_callback test_callback_;
};

TEST_F(UserRegisteredTest, Constuctor) {
	tr::user_registered cmd(1, "test", test_callback_);
}