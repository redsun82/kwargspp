# kwargspp

This is an implementation of keyword arguments in C++20.

## Basic example

Keywords need to be declared with `KWARGSPP_KEYWORDS`, for example

```cpp
KWARGSPP_KEYWORDS(_foo, _bar, _baz);
```

Then you can declare functions taking in such arguments as so:

```cpp
template <typename... Kwargs>
auto frobinize(Kwargs &&...kwargs) {
  auto [foo, bar, baz] =
      kwargspp::sig{_foo, _bar, _baz}(std::forward<Kwargs>(kwargs)...);
  // ...
}
```

All the follwing calls will work as expected:

```cpp
frobinize(_foo = 42, _bar = 3, _baz = "hello");
frobinize(_bar = 3, _foo = "hello", _baz = "world");
frobinize(_baz = "world", _foo = "hello", _bar = 3);
// frobinize(_baz = "world", _foo = "hello", _bar = 3, _foo = "!");  <- repeated keyword, fails at compile time
```

## Types

You can enforce specific typing using `keyword.as<Type>`. For example

```cpp
template <typename... Kwargs>
auto frobinize(Kwargs &&...kwargs) {
  auto [foo, bar, baz] =
      kwargspp::sig{_foo.as<int>, _bar.as<long>,
                    _baz.as<std::string_view>}(std::forward<Kwargs>(kwargs)...);
  // ...
```

Then on usage:

```cpp
frobinize(_foo = 42, _bar = 3, _baz = "hello");  // <- converts "hello" to "hello"sv
// frobinize(_bar = 3, _foo = "hello", _baz = "world"); <- fails at compile time
```

## Default values

In `kwargspp::sig` you can use `=` to assign default values.

```cpp
template <typename... Kwargs>
auto frobinize(Kwargs &&...kwargs) {
  auto [foo, bar, baz] = kwargspp::sig{_foo, _bar = 2, _baz = "hello"}(
      std::forward<Kwargs>(kwargs)...);
  // ...
```

Then on usage:

```cpp
frobinize(_baz = "hi", _foo = 42);  // same as frobinize(_foo = 42, _bar = 2, _baz = "hi")
frobinize(_foo = 1);  // same as frobinize(_foo = 1, _bar = 2, _baz = "hello")
```

Notice that a default value does not enforce a specific type:

```cpp
frobinize(_foo = 1, _baz = 42);  // same as frobinize(_foo = 1,_bar = 2, _baz = 42)
```

You can however combine typing and default values:

```cpp
template <typename... Kwargs>
auto frobinize(Kwargs &&...kwargs) {
  auto [foo, bar, baz] = kwargspp::sig{_foo.as<int>, _bar.as<long> = 2, _baz.as<std::string_view> = "hello"}(
      std::forward<Kwargs>(kwargs)...);
  // ...
```
