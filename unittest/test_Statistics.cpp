// Copyright (C) 2011-2020 Joel Rosdahl and other contributors
//
// See doc/AUTHORS.adoc for a complete list of contributors.
//
// This program is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option)
// any later version.
//
// This program is distributed in the hope that it will be useful, but WITHOUT
// ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
// FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
// more details.
//
// You should have received a copy of the GNU General Public License along with
// this program; if not, write to the Free Software Foundation, Inc., 51
// Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA

#include "../src/Statistic.hpp"
#include "../src/Statistics.hpp"
#include "../src/Util.hpp"
#include "../src/fmtmacros.hpp"
#include "TestUtil.hpp"

#include "third_party/doctest.h"

using TestUtil::TestContext;

TEST_SUITE_BEGIN("Statistics");

TEST_CASE("Read nonexistent")
{
  TestContext test_context;

  Counters counters = Statistics::read("test");

  REQUIRE(counters.size() == static_cast<size_t>(Statistic::END));
  CHECK(counters.get(Statistic::cache_miss) == 0);
}

TEST_CASE("Read bad")
{
  TestContext test_context;

  Util::write_file("test", "bad 1 2 3 4 5\n");
  Counters counters = Statistics::read("test");

  REQUIRE(counters.size() == static_cast<size_t>(Statistic::END));
  CHECK(counters.get(Statistic::cache_miss) == 0);
}

TEST_CASE("Read existing")
{
  TestContext test_context;

  Util::write_file("test", "0 1 2 3 27 5\n");
  Counters counters = Statistics::read("test");

  REQUIRE(counters.size() == static_cast<size_t>(Statistic::END));
  CHECK(counters.get(Statistic::cache_miss) == 27);
  CHECK(counters.get(Statistic::could_not_use_modules) == 0);
}

TEST_CASE("Read future counters")
{
  TestContext test_context;

  std::string content;
  size_t count = static_cast<size_t>(Statistic::END) + 1;
  for (size_t i = 0; i < count; ++i) {
    content += FMT("{}\n", i);
  }

  Util::write_file("test", content);
  Counters counters = Statistics::read("test");

  REQUIRE(counters.size() == count);
  for (size_t i = 0; i < count; ++i) {
    CHECK(counters.get_raw(i) == i);
  }
}

TEST_CASE("Read log")
{
  TestContext test_context;

  Util::write_file("stats.log", "# comment\ndirect_cache_hit\n");
  Counters counters = Statistics::read_log("stats.log");

  CHECK(counters.get(Statistic::direct_cache_hit) == 1);
  CHECK(counters.get(Statistic::cache_miss) == 0);
}

TEST_CASE("Update")
{
  TestContext test_context;

  Util::write_file("test", "0 1 2 3 27 5\n");

  auto counters = Statistics::update("test", [](Counters& cs) {
    cs.increment(Statistic::internal_error, 1);
    cs.increment(Statistic::cache_miss, 6);
  });
  REQUIRE(counters);

  CHECK(counters->get(Statistic::internal_error) == 4);
  CHECK(counters->get(Statistic::cache_miss) == 33);

  counters = Statistics::read("test");
  CHECK(counters->get(Statistic::internal_error) == 4);
  CHECK(counters->get(Statistic::cache_miss) == 33);
}

TEST_CASE("Get result")
{
  TestContext test_context;

  auto counters = Statistics::update(
    "test", [](Counters& cs) { cs.increment(Statistic::cache_miss, 1); });
  REQUIRE(counters);

  auto result = Statistics::get_result_message(*counters);
  REQUIRE(result);
}

TEST_CASE("Log result")
{
  TestContext test_context;

  auto counters = Statistics::update(
    "test", [](Counters& cs) { cs.increment(Statistic::cache_miss, 1); });
  REQUIRE(counters);

  auto result_id = Statistics::get_result_id(*counters);
  REQUIRE(result_id);
  Statistics::log_result("stats.log", "test.c", *result_id);

  auto statslog = Util::read_file("stats.log");
  REQUIRE(statslog.find(*result_id + "\n") != std::string::npos);
}

TEST_SUITE_END();
