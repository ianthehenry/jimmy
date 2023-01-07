#include <janet.h>
#include <immer/set.hpp>
#include <immer/set_transient.hpp>

#define CAST_SET(expr) static_cast<immer::set<Janet> *>(expr)
#define CAST_SET_ITERATOR(expr) static_cast<immer::set<Janet>::iterator *>(expr)
#define NEW_SET() new (janet_abstract(&set_type, sizeof(immer::set<Janet>))) immer::set<Janet>()

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

static const JanetAbstractType set_iterator_type = {
  "jimmy/set-iterator",
  JANET_ATEND_NAME
};

static int set_gc(void *data, size_t len) {
  (void) len;
  auto set = CAST_SET(data);
  set->~set();
  return 0;
}

static int set_gcmark(void *data, size_t len) {
  (void) len;
  auto set = CAST_SET(data);
  for (auto el : *set) {
    janet_mark(el);
  }
  return 0;
}

static void set_tostring(void *data, JanetBuffer *buffer) {
  auto set = CAST_SET(data);
  janet_buffer_push_cstring(buffer, "{");
  int first = 1;
  for (auto el : *set) {
    if (first) {
      first = 0;
    } else {
      janet_buffer_push_cstring(buffer, " ");
    }
    janet_pretty(buffer, 0, 0, el);
  }
  janet_buffer_push_cstring(buffer, "}");
}

static Janet cfun_set_length(int32_t argc, Janet *argv) {
  janet_fixarity(argc, 1);
  auto set = CAST_SET(janet_unwrap_abstract(argv[0]));
  return janet_wrap_integer(static_cast<int32_t>(set->size()));
}

static const JanetMethod set_methods[] = {
  {"length", cfun_set_length},
  {NULL, NULL}
};

static int set_get(void *data, Janet key, Janet *out) {
  if (janet_checkabstract(key, &set_iterator_type)) {
    *out = **CAST_SET_ITERATOR(janet_unwrap_abstract(key));
    return 1;
  } else if (janet_checktype(key, JANET_KEYWORD)) {
    return janet_getmethod(janet_unwrap_keyword(key), set_methods, out);
  } else {
    return 0;
  }
}

static Janet set_next(void *data, Janet key) {
  auto set = CAST_SET(data);

  if (janet_checktype(key, JANET_NIL)) {
    auto iterator = CAST_SET_ITERATOR(janet_abstract(&set_iterator_type, sizeof(immer::set<Janet>::iterator)));
    *iterator = set->begin();
    return janet_wrap_abstract(iterator);
  }

  if (!janet_checkabstract(key, &set_iterator_type)) {
    janet_panicf("set key should be an iterator; got %v", key);
  }
  auto iterator = CAST_SET_ITERATOR(janet_unwrap_abstract(key));
  (*iterator)++;
  if (*iterator == set->end()) {
    return janet_wrap_nil();
  } else {
    return key;
  }
}

static Janet set_call(void *data, int32_t argc, Janet *argv) {
  janet_fixarity(argc, 1);
  auto set = CAST_SET(data);
  return janet_wrap_boolean(set->count(argv[0]));
}

static void set_marshal(void *data, JanetMarshalContext *ctx) {
  janet_marshal_abstract(ctx, data);
  auto set = CAST_SET(data);
  janet_marshal_int(ctx, static_cast<int32_t>(set->size()));
  for (auto el : *set) {
    janet_marshal_janet(ctx, el);
  }
}

static void *set_unmarshal(JanetMarshalContext *ctx) {
  auto set = CAST_SET(janet_unmarshal_abstract(ctx, sizeof(immer::set<Janet>)));
  new (set) immer::set<Janet>();
  auto transient = set->transient();
  int32_t size = janet_unmarshal_int(ctx);
  for (int32_t i = 0; i < size; i++) {
    transient.insert(janet_unmarshal_janet(ctx));
  }
  *set = transient.persistent();
  return set;
}

static int set_compare(void *data1, void *data2) {
  auto set1 = CAST_SET(data1);
  auto set2 = CAST_SET(data2);
  if (*set1 == *set2) {
    return 0;
  }
  return set1 > set2 ? 1 : -1;
}

static int32_t hash_mix(int32_t input, int32_t other) {
  return input ^ (other + 0x7BA481B5 + (input << 6) + (input >> 2));
}

static int32_t set_hash(void *data, size_t len) {
  (void) len;
  auto set = CAST_SET(data);
  // start with a random permutation of 16 1s and 16 0s
  uint32_t hash = 0x7EB6D401;
  // Iteration is order is stable over hash values, regardless
  // of how you construct the set.
  // https://github.com/arximboldi/immer/discussions/249#discussioncomment-4620734
  for (auto el : *set) {
    hash = hash_mix(hash, static_cast<int32_t>(std::hash<Janet>()(el)));
  }
  return hash;
}

static const JanetAbstractType set_type = {
  .name = "jimmy/set",
  .gc = set_gc,
  .gcmark = set_gcmark,
  .get = set_get,
  .put = NULL,
  .marshal = set_marshal,
  .unmarshal = set_unmarshal,
  .tostring = set_tostring,
  .compare = set_compare,
  .hash = set_hash,
  .next = set_next,
  .call = set_call,
};

static Janet cfun_set_new(int32_t argc, Janet *argv) {
  auto set = NEW_SET();
  auto transient = set->transient();
  for (int32_t i = 0; i < argc; i++) {
    transient.insert(argv[i]);
  }
  *set = transient.persistent();
  return janet_wrap_abstract(set);
}

static Janet cfun_set_add(int32_t argc, Janet *argv) {
  janet_arity(argc, 1, -1);
  auto old_set = CAST_SET(janet_getabstract(argv, 0, &set_type));
  auto new_set = NEW_SET();

  int32_t new_elements = argc - 1;
  if (new_elements == 1) {
    *new_set = old_set->insert(argv[1]);
  } else {
    auto transient = old_set->transient();
    for (int32_t i = 1; i < argc; i++) {
      transient.insert(argv[i]);
    }
    *new_set = transient.persistent();
  }
  return janet_wrap_abstract(new_set);
}

static Janet cfun_set_remove(int32_t argc, Janet *argv) {
  janet_arity(argc, 1, -1);
  auto old_set = CAST_SET(janet_getabstract(argv, 0, &set_type));
  auto new_set = NEW_SET();

  int32_t erased_elements = argc - 1;
  if (erased_elements == 1) {
    *new_set = old_set->erase(argv[1]);
  } else {
    auto transient = old_set->transient();
    for (int32_t i = 1; i < argc; i++) {
      transient.erase(argv[i]);
    }
    *new_set = transient.persistent();
  }
  return janet_wrap_abstract(new_set);
}

static Janet cfun_set_union(int32_t argc, Janet *argv) {
  auto new_set = NEW_SET();
  auto transient = new_set->transient();
  for (int32_t i = 0; i < argc; i++) {
    auto old_set = CAST_SET(janet_getabstract(argv, i, &set_type));
    for (auto el : *old_set) {
      transient.insert(el);
    }
  }
  *new_set = transient.persistent();
  return janet_wrap_abstract(new_set);
}

static immer::set<Janet> *intersect2(immer::set<Janet> *a, immer::set<Janet> *b) {
  auto new_set = NEW_SET();
  auto transient = new_set->transient();
  for (auto el : *a) {
    if (b->count(el)) {
      transient.insert(el);
    }
  }
  return new_set;
}

// We could specialize this to count elements in a temporary map when argc > 2
// but, you know, this library is not making any guarantees about time complexity.
static Janet cfun_set_intersection(int32_t argc, Janet *argv) {
  if (argc == 0) {
    return janet_wrap_abstract(NEW_SET());
  } else {
    auto first_set = CAST_SET(janet_getabstract(argv, 0, &set_type));
    for (int32_t i = 1; i < argc; i++) {
      first_set = intersect2(first_set, CAST_SET(janet_getabstract(argv, i, &set_type)));
    }
    return janet_wrap_abstract(first_set);
  }
}

static Janet cfun_set_difference(int32_t argc, Janet *argv) {
  janet_arity(argc, 1, -1);
  auto new_set = NEW_SET();
  auto transient = new_set->transient();
  for (int32_t i = 0; i < argc; i++) {
    auto old_set = CAST_SET(janet_getabstract(argv, i, &set_type));
    if (i == 0) {
      *new_set = *old_set;
    } else {
      for (auto el : *old_set) {
        transient.erase(el);
      }
    }
  }
  *new_set = transient.persistent();
  return janet_wrap_abstract(new_set);
}

static const JanetReg cfuns[] = {
  {"set/new", cfun_set_new, "(set/new & xs)\n\n"
    "Returns a persistent immutable set containing only the listed elements."},
  {"set/add", cfun_set_add, "(set/add set & xs)\n\n"
    "Returns a set containing the union of the original set and the rest of the arguments."},
  {"set/remove", cfun_set_remove, "(set/remove set & xs)\n\n"
    "Returns a set containing the elements from the original set without any of the arguments."},
  {"set/union", cfun_set_union, "(set/union & sets)\n\n"
    "Returns a set that is the union of all of its arguments."},
  {"set/intersection", cfun_set_intersection, "(set/intersection & sets)\n\n"
    "Returns a set that is the intersection of all of its arguments. Naively folds when given more than two arguments."},
  {"set/difference", cfun_set_difference, "(set/difference set & sets)\n\n"
    "Returns a set that is the first set minus all of the latter sets."},
  {NULL, NULL, NULL}
};

JANET_MODULE_ENTRY(JanetTable *env) {
  janet_cfuns(env, "jimmy/set", cfuns);
  janet_register_abstract_type(&set_type);
  janet_register_abstract_type(&set_iterator_type);
  janet_def(env, "set/empty", cfun_set_new(0, NULL), NULL);
}
