#include "gtest/gtest.h"

#include "traders_rating/cmds.h"
#include "traders_rating/service.h"

namespace tr = ::traders_rating;

TEST(ServiceTest, Create) {
	try {
		tr::service srv;
	} catch(std::exception& e) {
		FAIL() << e.what();
	}
}

TEST(ServiceTest, StartStop) {
	try {
		tr::service srv;
		srv.start();
		srv.stop();
	} catch(std::exception& e) {
		FAIL() << e.what();
	}
}
