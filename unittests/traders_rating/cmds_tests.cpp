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

	void test_callback(tr::user_id_t id, const tr::user_name_t& name) {
		called++;
		user_id = id;
		user_name = name;
	}

	tr::user_registered_callback test_callback_;
	tr::user_id_t user_id = 0;
	tr::user_name_t	user_name = "";
	unsigned called = 0;
};

TEST_F(UserRegisteredTest, Constuctor) {
	try {
		tr::user_registered cmd(1, "test", test_callback_);
		SUCCEED();
	} catch(std::exception& e) {
		FAIL() << e.what();
	}
}

TEST_F(UserRegisteredTest, Callback) {
	try {
		tr::cmd_uptr cmd(new tr::user_registered(10, "test11", test_callback_));
		cmd->handle();
		ASSERT_EQ(called, 1);
		ASSERT_EQ(user_id, 10);
		ASSERT_STREQ(user_name.c_str(), "test11");
	} catch(std::exception& e) {
		FAIL() << e.what();
	}
}


TEST(UserRenameTest, Callback) {
	try {
		tr::user_id_t test_id = 0;
		tr::user_name_t test_name = "";
		unsigned called = 0;

		tr::cmd_uptr cmd(new tr::user_renamed(20, "test12", 
			[&](tr::user_id_t id, const tr::user_name_t& name){
				test_id = id;
				test_name = name;
				++called;
			}));
		cmd->handle();
		ASSERT_EQ(called, 1);
		ASSERT_EQ(test_id, 20);
		ASSERT_STREQ(test_name.c_str(), "test12");
	} catch(std::exception& e) {
		FAIL() << e.what();
	}
}


TEST(UserConnectedTest, Callback) {
	try {
		tr::user_id_t test_id = 0;
		unsigned called = 0;

		tr::cmd_uptr cmd(new tr::user_connected(20, 
			[&](tr::user_id_t id){
				test_id = id;
				++called;
			}));
		cmd->handle();
		ASSERT_EQ(called, 1);
		ASSERT_EQ(test_id, 20);
	} catch(std::exception& e) {
		FAIL() << e.what();
	}
}
