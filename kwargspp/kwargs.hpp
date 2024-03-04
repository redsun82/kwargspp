#pragma once

#include <concepts>
#include <tuple>
#include <type_traits>
#include <utility>

namespace kwargspp {

template <typename Derived, typename RequiredType = void> struct keyword;

template <typename T>
concept UntypedKeyword = std::derived_from<T, keyword<T>>;

template <typename T>
concept TypedKeyword = requires {
  typename T::untyped_keyword_type;
  typename T::required_type;
  requires std::same_as<
      T, keyword<typename T::untyped_keyword_type, typename T::required_type>>;
};

template <typename T>
concept TypedOrUntypedKeyword = UntypedKeyword<T> || TypedKeyword<T>;

template <typename T>
concept Keyword =
    TypedOrUntypedKeyword<std::remove_const_t<std::remove_reference_t<T>>>;

template <Keyword Kw, typename T> struct keyword_parameter {
  using untyped_keyword_type = typename Kw::untyped_keyword_type;
  using keyword_type = Kw;
  using value_type = T &&;
  using required_type = typename Kw::required_type;

  static constexpr bool has_required_type = Kw::has_required_type;

  T &&value;
};

template <typename T>
concept KeywordParameter = requires {
  typename T::keyword_type;
  typename T::value_type;
  requires std::same_as<
      T, keyword_parameter<typename T::keyword_type, typename T::value_type>>;
};

namespace detail {
template <typename T, typename RequiredType> struct type_rule_impl {
  static constexpr bool value = std::convertible_to<T &&, RequiredType &&>;
};

template <typename T> struct type_rule_impl<T, void> : std::true_type {};
} // namespace detail

template <typename Derived, typename RequiredType> struct keyword {
  using untyped_keyword_type = Derived;
  using keyword_type = keyword;
  using required_type = RequiredType;
  static constexpr bool has_required_type = !std::same_as<RequiredType, void>;
  keyword &operator=(const keyword &) = delete;

  template <typename T>
  static constexpr bool accepts_type =
      detail::type_rule_impl<T, RequiredType>::value;

  template <typename T>
    requires accepts_type<T>
  keyword_parameter<Derived, T &&> operator=(T &&value) const {
    return {std::forward<T>(value)};
  }

  template <typename T>
    requires(!has_required_type)
  static constexpr keyword<Derived, T> as{};
};

template <typename T>
concept KeywordSpec = Keyword<T> || KeywordParameter<T>;

template <KeywordSpec Kw>
using keyword_type_of = typename std::remove_reference_t<Kw>::keyword_type;

template <KeywordSpec Kw>
using untyped_keyword_type_of =
    typename std::remove_reference_t<Kw>::untyped_keyword_type;

template <KeywordSpec Kw>
using required_type_of = typename std::remove_reference_t<Kw>::required_type;

template <KeywordParameter Kwarg>
using value_type_of = typename std::remove_reference_t<Kwarg>::value_type;

template <KeywordSpec Kwarg, typename T>
inline constexpr bool accepts_type =
    keyword_type_of<Kwarg>::template accepts_type<T>;

#define KWARGSPP_KEYWORD(name)                                                 \
  struct name##_type : ::kwargspp::keyword<name##_type> {                      \
    using keyword<name##_type>::operator=;                                     \
    using keyword<name##_type>::as;                                            \
    using keyword<name##_type>::keyword_type;                                  \
    using keyword<name##_type>::required_type;                                 \
    using keyword<name##_type>::has_required_type;                             \
  };                                                                           \
  constexpr name##_type name {}

#define KWARGSPP_KEYWORD_SEMICOLON(name) KWARGSPP_KEYWORD(name);

#define PARENS ()

#define EXPAND(...) EXPAND4(EXPAND4(EXPAND4(EXPAND4(__VA_ARGS__))))
#define EXPAND4(...) EXPAND3(EXPAND3(EXPAND3(EXPAND3(__VA_ARGS__))))
#define EXPAND3(...) EXPAND2(EXPAND2(EXPAND2(EXPAND2(__VA_ARGS__))))
#define EXPAND2(...) EXPAND1(EXPAND1(EXPAND1(EXPAND1(__VA_ARGS__))))
#define EXPAND1(...) __VA_ARGS__

#define FOR_EACH(macro, ...)                                                   \
  __VA_OPT__(EXPAND(FOR_EACH_HELPER(macro, __VA_ARGS__)))
#define FOR_EACH_HELPER(macro, a1, ...)                                        \
  macro(a1) __VA_OPT__(FOR_EACH_AGAIN PARENS(macro, __VA_ARGS__))
#define FOR_EACH_AGAIN() FOR_EACH_HELPER

#define KWARGSPP_KEYWORDS(...)                                                 \
  FOR_EACH(KWARGSPP_KEYWORD_SEMICOLON, __VA_ARGS__) static_assert(true)

namespace detail {
template <typename... Ts>
using first_of = typename std::tuple_element<0, std::tuple<Ts...>>::type;

template <typename F, typename T, typename... Ts>
inline auto invoke_on_rest(F &&f, T &&, Ts &&...args) {
  return std::forward<F>(f)(std::forward<Ts>(args)...);
}

template <typename T, typename... Ts>
inline auto get_first(T &&value, Ts &&...) {
  return std::forward<T>(value);
}

template <typename T, typename... Ts>
inline auto get_rest(T &&, Ts &&...values) {
  return std::forward_as_tuple(std::forward<Ts>(values)...);
}

struct NotFound {};

template <KeywordSpec Kw, KeywordParameter... Kwargs>
decltype(auto) get(Kw &&kw, Kwargs &&...kwargs) {
  if constexpr (sizeof...(Kwargs) == 0) {
    if constexpr (KeywordParameter<Kw>) {
      return std::forward<value_type_of<Kw>>(std::forward<Kw>(kw).value);
    } else {
      return NotFound{};
    }
  } else {
    using Kwarg = first_of<Kwargs...>;
    auto recursive = [&](auto &&arg) {
      return std::apply(
          [&arg](auto &&...args) {
            return get(std::forward<decltype(arg)>(arg),
                       std::forward<decltype(args)>(args)...);
          },
          get_rest(std::forward<Kwargs>(kwargs)...));
    };
    if constexpr (std::same_as<untyped_keyword_type_of<Kw>,
                               untyped_keyword_type_of<Kwarg>>) {
      static_assert(accepts_type<Kw, value_type_of<Kwarg>>, "mistyping!");
      static_assert(
          std::same_as<decltype(recursive(untyped_keyword_type_of<Kw>{})),
                       NotFound>,
          "multiple keyword usage");
      return std::forward<value_type_of<Kwarg>>(
          get_first(std::forward<Kwargs>(kwargs)...).value);
    } else {
      return recursive(std::forward<Kw>(kw));
    }
  }
}
} // namespace detail

template <KeywordSpec... Kws> struct sig {
  std::tuple<Kws &&...> keywords;

public:
  sig(Kws &&...keywords) : keywords(std::forward<Kws>(keywords)...) {}

  template <KeywordParameter... Kwargs> auto operator()(Kwargs &&...kwargs) && {
    return std::apply(
        [&](auto &&...args) {
          return std::tuple{detail::get(std::forward<decltype(args)>(args),
                                        std::forward<Kwargs>(kwargs)...)...};
        },
        std::move(keywords));
  }
};

template <KeywordSpec... Kws> sig(Kws &&...) -> sig<Kws...>;

} // namespace kwargspp
