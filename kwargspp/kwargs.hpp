#pragma once

#include <concepts>
#include <tuple>
#include <type_traits>
#include <utility>

namespace kwargspp {

template <typename Kw, typename T> struct KeywordParameter {
  using keyword_type = Kw;
  T value;
};

template <typename Derived, typename T> struct TypedKeyword {
  TypedKeyword operator=(const TypedKeyword &) = delete;

  template <std::convertible_to<T> U>
  KeywordParameter<Derived, T> operator=(U &&value) const {
    return {std::forward<U>(value)};
  }
};

template <typename Derived> struct Keyword {
  Keyword operator=(const Keyword &) = delete;

  template <typename T>
  KeywordParameter<Derived, T> operator=(T &&value) const {
    return {std::forward<T>(value)};
  }

  template <typename T> static constexpr TypedKeyword<Derived, T> as{};
};

#define KWARGSPP_KEYWORD(name)                                                 \
  struct name##_keyword_type : ::kwargspp::Keyword<name##_keyword_type> {      \
    using Keyword<name##_keyword_type>::operator=;                             \
    using Keyword<name##_keyword_type>::as;                                    \
  };                                                                           \
  constexpr name##_keyword_type name {}

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

#define KWARGSPP_KEYWORDS(...) FOR_EACH(KWARGSPP_KEYWORD_SEMICOLON, __VA_ARGS__) static_assert(true)

    namespace detail {
  template <typename Kw, typename... Kwargs> struct GetImpl;

  struct NotFound {};

  template <typename Kw, typename T, typename... Kwargs>
  struct GetImpl<Kw, KeywordParameter<Kw, T>, Kwargs...> {
    using Kwarg = KeywordParameter<Kw, T>;
    static auto get(Kwarg &&first, Kwargs &&...rest) {
      if constexpr (std::is_same_v<decltype(get_from_rest(
                                       std::forward<Kwargs>(rest)...)),
                                   NotFound>) {
        return std::forward<Kwarg>(first).value;
      } else {
        return get_from_rest(std::forward<Kwargs>(rest)...);
      }
    };

  private:
    static auto get_from_rest(Kwargs &&...rest) {
      return GetImpl<Kw, Kwargs...>::get(std::forward<Kwargs>(rest)...);
    }
  };

  template <typename Kw, typename T, typename U>
  inline constexpr bool is_correctly_typed = std::is_constructible_v<T, U>;

  template <typename Kw, typename T, typename U, typename... Kwargs>
  struct GetImpl<TypedKeyword<Kw, T>, KeywordParameter<Kw, U>, Kwargs...> {
    using Kwarg = KeywordParameter<Kw, U>;
    static auto get(Kwarg &&first, Kwargs &&...rest) {
      static_assert(is_correctly_typed<Kw, T, U>, "Wrong type");
      if constexpr (std::is_same_v<decltype(get_from_rest(
                                       std::forward<Kwargs>(rest)...)),
                                   NotFound>) {
        return T{std::forward<Kwarg>(first).value};
      } else {
        return get_from_rest(std::forward<Kwargs>(rest)...);
      }
    };

  private:
    static auto get_from_rest(Kwargs &&...rest) {
      return GetImpl<Kw, Kwargs...>::get(std::forward<Kwargs>(rest)...);
    }
  };

  template <typename Kw, typename Kwarg, typename... Kwargs>
  struct GetImpl<Kw, Kwarg, Kwargs...> {
    static auto get(Kwarg &&first, Kwargs &&...rest) {
      return GetImpl<Kw, Kwargs...>::get(std::forward<Kwargs>(rest)...);
    };
  };

  template <typename Kw> struct GetImpl<Kw> {
    static NotFound get();
  };
} // namespace detail

template <auto Kw, typename... Kwargs> auto get(Kwargs &&...kwargs) {
  return detail::GetImpl<std::remove_const_t<decltype(Kw)>, Kwargs...>::get(
      std::forward<Kwargs>(kwargs)...);
}

namespace detail {
template <auto... Kws> struct Signature {
  template <typename... DefaultKwargs> struct Getter {
    std::tuple<DefaultKwargs...> default_args;
    template <typename... Kwargs> auto apply(Kwargs &&...kwargs) && {
      return std::tuple{std::apply(
          []<typename... Args>(Args &&...args) {
            return get<Kws>(std::forward<Args>(args)...);
          },
          std::tuple_cat(
              std::move(default_args),
              std::forward_as_tuple(std::forward<Kwargs>(kwargs)...)))...};
    }

    template <typename Kwarg> auto prependDefaultKwarg(Kwarg &&kwarg) && {
      using G =
          typename Signature<typename Kwarg::keyword_type{},
                             Kws...>::template Getter<Kwarg, DefaultKwargs...>;
      return G{std::tuple_cat(std::forward_as_tuple(std::forward<Kwarg>(kwarg)),
                              std::move(default_args))};
    }
    template <auto Kw> auto prependKw() && {
      using G =
          typename Signature<Kw, Kws...>::template Getter<DefaultKwargs...>;
      return G{std::move(default_args)};
    }
  };
  template <typename... DefaultKwargs>
  static auto with_defaults(DefaultKwargs &&...default_args) {
    return Getter<DefaultKwargs...>{
        {std::forward<DefaultKwargs>(default_args)...}};
  }
};

template <typename... KwOrKwargs> struct SignatureBuilder;

template <typename KwOrKwarg, typename... KwOrKwargs>
struct SignatureBuilder<KwOrKwarg, KwOrKwargs...> {
  static auto create(KwOrKwarg &&kwOrKwarg, KwOrKwargs... rest) {
    if constexpr (requires { typename KwOrKwarg::keyword_type; }) {
      return SignatureBuilder<KwOrKwargs...>::create(
                 std::forward<KwOrKwargs>(rest)...)
          .template prependDefaultKwarg(std::move(kwOrKwarg));
    } else {
      return SignatureBuilder<KwOrKwargs...>::create(
                 std::forward<KwOrKwargs>(rest)...)
          .template prependKw<KwOrKwarg{}>();
    }
  }
};

template <> struct SignatureBuilder<> {
  static auto create() { return Signature<>::template Getter<>{{}}; }
};
} // namespace detail

template <typename... KwOrKwargs> auto signature(KwOrKwargs... kwargs) {
  return detail::SignatureBuilder<KwOrKwargs...>::create(
      std::forward<KwOrKwargs>(kwargs)...);
}

} // namespace kwargspp
