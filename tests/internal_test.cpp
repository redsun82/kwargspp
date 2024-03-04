#include "kwargspp/kwargs.hpp"

#include <catch2/catch_all.hpp>
#include <iostream>
#include <memory>
#include <string_view>

using namespace std::literals;
using namespace kwargspp;

KWARGSPP_KEYWORDS(_foo, _bar, _baz, _bazz);

static_assert(Keyword<decltype(_foo)>);
static_assert(Keyword<decltype(_foo.as<int>)>);
static_assert(Keyword<const decltype(_foo.as<int>) &>);
static_assert(KeywordSpec<decltype(_foo)>);
static_assert(KeywordSpec<decltype(_foo.as<int>)>);
static_assert(KeywordSpec<decltype(_foo.as<int>)>);
static_assert(KeywordSpec<decltype(_foo = 42)>);
static_assert(KeywordSpec<decltype(_foo.as<int> = 42)>);

static_assert(std::convertible_to<decltype("hello"), std::string_view>);

template <typename... Ts> void debug_print(Ts &&...args) {
  std::cerr << __PRETTY_FUNCTION__ << std::endl;
}

TEST_CASE("test") {
  CHECK(detail::get(_foo, _foo = 42) == 42);
  CHECK(detail::get(_foo.as<long>, _foo = 42) == 42L);
  CHECK(detail::get(_foo = 42) == 42);
  CHECK(detail::get(_foo.as<long> = 42) == 42L);
  long i;
  CHECK(&detail::get(_foo.as<long &>, _foo = i) == &i);
  auto x = std::make_shared<int>(42);
  auto raw_x = x.get();
  auto y = detail::get(_foo.as<std::shared_ptr<int> &&>, _foo = std::move(x));
  CHECK(y.get() == raw_x);
  CHECK(x == nullptr);

  CHECK(detail::get(_foo, _foo = 42, _bar = 3, _baz = "") == 42);
  CHECK(detail::get(_foo, _bar = 3, _foo = "hello"sv, _baz = "") == "hello"sv);
}
