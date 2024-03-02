#include "kwargs.hpp"

#include <catch2/catch_all.hpp>

#include <sstream>
#include <string>

KWARGSPP_KEYWORDS(_foo, _bar, _baz);

template <typename... Kwargs> std::string frobinize(Kwargs &&...kwargs) {
  auto [foo, bar, baz] = kwargspp::sig{_foo = 1, _bar = 2, _baz = "hello"}(
      std::forward<Kwargs>(kwargs)...);

  std::ostringstream os;
  os << "foo: " << foo << ", bar: " << bar << ", baz: " << baz;
  return std::move(os).str();
}

TEST_CASE("frobinize with defaults") {
  CHECK(frobinize(_foo = 42, _bar = 3, _baz = "hi") ==
        "foo: 42, bar: 3, baz: hi");
  CHECK(frobinize(_baz = "hi", _foo = 42) == "foo: 42, bar: 2, baz: hi");
  CHECK(frobinize() == "foo: 1, bar: 2, baz: hello");
  CHECK(frobinize(_baz = 42) == "foo: 1, bar: 2, baz: 42");
}
