#include <catch2/catch_all.hpp>

#include "kwargspp/kwargs.hpp"

#include <iostream>
#include <string_view>

using namespace std::literals;

KWARGSPP_KEYWORDS(  _foo, _bar, _baz, _bazz);

TEST_CASE("test") {
  CHECK(kwargspp::get<_foo>(_foo = 42) == 42);
  CHECK(kwargspp::get<_foo.as<long>>(_foo = 42) == 42L);
  CHECK(kwargspp::get<_foo>(_foo = 42, _bar = 3, _baz = "") == 42);
  CHECK(kwargspp::get<_foo>(_bar = 3, _foo = "hello"sv,
                               _baz = "") == "hello"sv);
  CHECK(kwargspp::get<_foo>(_bar = 3, _foo = "hello"sv, _baz = "",
                               _foo = 42) == 42);

  std::string x = "hello";
  []<typename... Kwargs>(Kwargs &&...kwargs) {
    auto [foo, bar, baz, bazz] =
        signature(_foo, _bar = 5, _baz.as<std::string&&>,
                  _bazz.as<long> = 22L)
            .apply(std::forward<Kwargs>(kwargs)...);
    CHECK(foo == 3);
    CHECK(bar == 5);
    CHECK(baz == "hello");
    CHECK(bazz == 22L);
  }(_foo = 3, _baz = std::move(x));
}
