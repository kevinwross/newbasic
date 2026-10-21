#include <catch2/catch_test_macros.hpp>

#include <string>

#include "lwbasic/line_store.h"

using lwbasic::LineStore;

TEST_CASE("LineStore: ordering, replace, remove", "[line-store]") {
  LineStore store;
  REQUIRE(store.Add(20, "PRINT 2"));
  REQUIRE(store.Add(10, "PRINT 1"));
  REQUIRE(store.Add(30, "PRINT 3"));
  CHECK(store.Size() == 3);
  CHECK(store.Lines()[0].line_number == 10);
  CHECK(store.Lines()[1].line_number == 20);
  CHECK(store.Lines()[2].line_number == 30);
  CHECK(store.FindIndex(20).has_value());
  CHECK(*store.FindIndex(20) == 1);
  CHECK_FALSE(store.FindIndex(25).has_value());

  REQUIRE(store.Add(20, "PRINT 22")); // replace
  CHECK(store.Lines()[1].source == "PRINT 22");
  CHECK(store.Size() == 3);

  REQUIRE(store.Remove(10));
  CHECK(store.Lines()[0].line_number == 20);
  CHECK_FALSE(store.Remove(10));

  CHECK_FALSE(store.Add(0, "x"));
  CHECK_FALSE(store.Add(1000000, "x"));
}

TEST_CASE("LineStore::ParseTextBas", "[line-store]") {
  std::string error;
  LineStore store;

  SECTION("CRLF input") {
    REQUIRE(LineStore::ParseTextBas("10 PRINT \"A\"\r\n20 PRINT \"B\"\r\n", &store, &error));
    CHECK(store.Size() == 2);
    CHECK(store.Lines()[0].source == "PRINT \"A\"");
    CHECK(store.Lines()[1].line_number == 20);
  }
  SECTION("LF input, blank lines, tab separator") {
    REQUIRE(LineStore::ParseTextBas("\n10\tPRINT 1\n\n30 PRINT 3\n", &store, &error));
    CHECK(store.Size() == 2);
    CHECK(store.Lines()[0].line_number == 10);
    CHECK(store.Lines()[0].source == "PRINT 1");
    CHECK(store.Lines()[1].line_number == 30);
  }
  SECTION("duplicate line number") {
    LineStore t;
    REQUIRE_FALSE(LineStore::ParseTextBas("10 A\n10 B\n", &t, &error));
    CHECK(error.find("duplicate") != std::string::npos);
  }
  SECTION("missing line number (direct mode is M1)") {
    LineStore t;
    REQUIRE_FALSE(LineStore::ParseTextBas("PRINT 1\n", &t, &error));
    CHECK(error.find("missing line number") != std::string::npos);
  }
  SECTION("no separator after line number") {
    LineStore t;
    REQUIRE_FALSE(LineStore::ParseTextBas("10X=1\n", &t, &error));
    CHECK(error.find("space or tab") != std::string::npos);
  }
  SECTION("line number too large") {
    LineStore t;
    REQUIRE_FALSE(LineStore::ParseTextBas("1000000 X\n", &t, &error));
    CHECK(error.find("too large") != std::string::npos);
  }
}

TEST_CASE("LineStore: replace in place and bulk insert", "[line-store]") {
  LineStore store;
  REQUIRE(store.Add(10, "PRINT 1"));
  REQUIRE(store.Add(20, "PRINT 2"));
  // Adding an existing number replaces the text and keeps the position.
  REQUIRE(store.Add(10, "PRINT 111"));
  CHECK(store.Size() == 2);
  CHECK(store.Lines()[0].line_number == 10);
  CHECK(store.Lines()[0].source == "PRINT 111");
  // A hundred lines inserted newest-first still come back in ascending order.
  for (int i = 0; i < 100; ++i)
    REQUIRE(store.Add(2000 - i * 10, "REM x"));
  CHECK(store.Size() == 102);
  CHECK(store.Lines().front().line_number == 10);
  CHECK(store.Lines().back().line_number == 2000);
}
