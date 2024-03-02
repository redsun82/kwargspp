#include "kwargs.hpp"

#include <catch2/catch_all.hpp>

#include <sstream>
#include <string>
#include <string_view>

KWARGSPP_KEYWORDS(_foo, _bar, _baz);

template <typename... Kwargs> std::string frobinize(Kwargs &&...kwargs) {
  auto [foo, bar, baz] =
      kwargspp::sig{_foo.as<int>, _bar.as<long>,
                    _baz.as<std::string_view>}(std::forward<Kwargs>(kwargs)...);

  std::ostringstream os;
  os << "foo: " << foo << ", bar: " << bar << ", baz: " << baz;
  return std::move(os).str();
}

TEST_CASE("typed frobinize") {
  CHECK(frobinize(_foo = 42, _bar = 3, _baz = "hello") ==
        "foo: 42, bar: 3, baz: hello");
  // fails to compile:
  //   CHECK(frobinize(_bar = 3, _foo = "hello", _baz = "world") ==
  //         "foo: hello, bar: 3, baz: world");
  //   CHECK(frobinize(_baz = "world", _foo = "hello", _bar = 3) ==
  //         "foo: hello, bar: 3, baz: world");
}
