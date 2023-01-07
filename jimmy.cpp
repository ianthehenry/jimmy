#include <janet.h>
#include <immer/set.hpp>
#include <immer/set_transient.hpp>

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
  auto set = static_cast<immer::set<Janet> *>(data);
  set->~set();
  return 0;
}

static int set_gcmark(void *data, size_t len) {
  (void) len;
  auto set = static_cast<immer::set<Janet> *>(data);
  for (auto el : *set) {
    janet_mark(el);
  }
  return 0;
}

static void set_tostring(void *data, JanetBuffer *buffer) {
  auto set = static_cast<immer::set<Janet> *>(data);
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
  auto set = static_cast<immer::set<Janet> *>(janet_unwrap_abstract(argv[0]));
  return janet_wrap_integer(static_cast<int32_t>(set->size()));
}

static const JanetMethod set_methods[] = {
  {"length", cfun_set_length},
  {NULL, NULL}
};

static int set_get(void *data, Janet key, Janet *out) {
  if (janet_checkabstract(key, &set_iterator_type)) {
    *out = **static_cast<immer::set<Janet>::iterator *>(janet_unwrap_abstract(key));
    return 1;
  } else if (janet_checktype(key, JANET_KEYWORD)) {
    return janet_getmethod(janet_unwrap_keyword(key), set_methods, out);
  } else {
    return 0;
  }
}

static Janet set_next(void *data, Janet key) {
  auto set = static_cast<immer::set<Janet> *>(data);

  if (janet_checktype(key, JANET_NIL)) {
    auto iterator = static_cast<immer::set<Janet>::iterator *>(janet_abstract(&set_iterator_type, sizeof(immer::set<Janet>::iterator)));
    *iterator = set->begin();
    return janet_wrap_abstract(iterator);
  }

  if (!janet_checkabstract(key, &set_iterator_type)) {
    janet_panicf("set key should be an iterator; got %v", key);
  }
  auto iterator = static_cast<immer::set<Janet>::iterator *>(janet_unwrap_abstract(key));
  (*iterator)++;
  if (*iterator == set->end()) {
    return janet_wrap_nil();
  } else {
    return key;
  }
}

static Janet set_call(void *data, int32_t argc, Janet *argv) {
  janet_fixarity(argc, 1);
  auto set = static_cast<immer::set<Janet> *>(data);
  return janet_wrap_boolean(set->count(argv[0]));
}

static void set_marshal(void *data, JanetMarshalContext *ctx) {
  janet_marshal_abstract(ctx, data);
  auto set = static_cast<immer::set<Janet> *>(data);
  janet_marshal_int(ctx, static_cast<int32_t>(set->size()));
  for (auto el : *set) {
    janet_marshal_janet(ctx, el);
  }
}

static void *set_unmarshal(JanetMarshalContext *ctx) {
  auto set = static_cast<immer::set<Janet> *>(janet_unmarshal_abstract(ctx, sizeof(immer::set<Janet>)));
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
  auto set1 = static_cast<immer::set<Janet> *>(data1);
  auto set2 = static_cast<immer::set<Janet> *>(data2);
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
  auto set = static_cast<immer::set<Janet> *>(data);
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
  auto set = static_cast<immer::set<Janet> *>(janet_abstract(&set_type, sizeof(immer::set<Janet>)));
  new (set) immer::set<Janet>();
  auto transient = set->transient();
  for (int32_t i = 0; i < argc; i++) {
    transient.insert(argv[i]);
  }
  *set = transient.persistent();
  return janet_wrap_abstract(set);
}

static Janet cfun_set_add(int32_t argc, Janet *argv) {
  janet_arity(argc, 1, -1);
  auto old_set = static_cast<immer::set<Janet> *>(janet_getabstract(argv, 0, &set_type));
  auto new_set = static_cast<immer::set<Janet> *>(janet_abstract(&set_type, sizeof(immer::set<Janet>)));
  new (new_set) immer::set<Janet>();

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
  auto old_set = static_cast<immer::set<Janet> *>(janet_getabstract(argv, 0, &set_type));
  auto new_set = static_cast<immer::set<Janet> *>(janet_abstract(&set_type, sizeof(immer::set<Janet>)));
  new (new_set) immer::set<Janet>();

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

static const JanetReg cfuns[] = {
  {"set/new", cfun_set_new, NULL},
  {"set/add", cfun_set_add, NULL},
  {"set/remove", cfun_set_remove, NULL},
  {NULL, NULL, NULL}
};

JANET_MODULE_ENTRY(JanetTable *env) {
  janet_cfuns(env, "jimmy/set", cfuns);
  janet_register_abstract_type(&set_type);
  janet_register_abstract_type(&set_iterator_type);
  janet_def(env, "set/empty", cfun_set_new(0, NULL), NULL);
}
