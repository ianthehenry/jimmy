#include <janet.h>
#include <functional>

namespace std {
  template <> struct hash<Janet> {
    size_t operator()(const Janet &x) const {
      return static_cast<size_t>(janet_hash(x));
    }
  };

  template <> struct equal_to<Janet> {
    size_t operator()(const Janet &a, const Janet &b) const {
      return static_cast<bool>(janet_equals(a, b));
    }
  };
}

bool operator==(const Janet& a, const Janet& b) {
  return static_cast<bool>(janet_equals(a, b));
}

static int32_t hash_mix(int32_t input, int32_t other) {
  return input ^ (other + 0b01111011101001001000000110110101 + (input << 6) + (input >> 2));
}

static Janet pair_to_tuple(std::pair<Janet, Janet> pair) {
  Janet *tuple = janet_tuple_begin(2);
  tuple[0] = pair.first;
  tuple[1] = pair.second;
  return janet_wrap_tuple(janet_tuple_end(tuple));
}

// TODO: this is copied from `janet_method_invoke` -- there doesn't seem
// to be an exposed way to do this!
static Janet call_callable(Janet callable, int32_t argc, Janet *argv) {
  switch (janet_type(callable)) {
    case JANET_CFUNCTION:
      return (janet_unwrap_cfunction(callable))(argc, argv);
    case JANET_FUNCTION: {
      JanetFunction *fun = janet_unwrap_function(callable);
      return janet_call(fun, argc, argv);
    }
    case JANET_ABSTRACT: {
      JanetAbstract abst = janet_unwrap_abstract(callable);
      const JanetAbstractType *at = janet_abstract_type(abst);
      if (NULL != at->call) {
        return at->call(abst, argc, argv);
      }
    }
    /* fallthrough */
    case JANET_STRING:
    case JANET_BUFFER:
    case JANET_TABLE:
    case JANET_STRUCT:
    case JANET_ARRAY:
    case JANET_TUPLE: {
      if (argc != 1) {
        janet_panicf("%v called with %d arguments, possibly expected 1", callable, argc);
      }
      return janet_in(callable, argv[0]);
    }
    default: {
      if (argc != 1) {
        janet_panicf("%v called with %d arguments, possibly expected 1", callable, argc);
      }
      return janet_in(argv[0], callable);
    }
  }
}

#include "set.cpp"
#include "map.cpp"

JANET_MODULE_ENTRY(JanetTable *env) {
  janet_cfuns(env, "jimmy", set_cfuns);
  janet_cfuns(env, "jimmy", map_cfuns);
  janet_register_abstract_type(&set_type);
  janet_register_abstract_type(&set_iterator_type);
  janet_register_abstract_type(&map_type);
  janet_register_abstract_type(&map_iterator_type);
}
