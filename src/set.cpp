#include <immer/set.hpp>
#include <immer/set_transient.hpp>

#define CAST_SET(expr) static_cast<immer::set<Janet> *>((expr))
#define CAST_SET_ITERATOR(expr) static_cast<SetIterator *>((expr))
#define NEW_SET() new (janet_abstract(&set_type, sizeof(immer::set<Janet>))) immer::set<Janet>()

#define CAST_TSET(expr) static_cast<immer::set_transient<Janet> *>((expr))
#define NEW_TSET() new (janet_abstract(&tset_type, sizeof(immer::set_transient<Janet>))) immer::set_transient<Janet>()

static int tset_gc(void *data, size_t len) {
  (void) len;
  auto tset = CAST_TSET(data);
  tset->~set_transient();
  return 0;
}

static const JanetAbstractType tset_type = {
  .name = "jimmy/tset",
  .gc = tset_gc,
  .gcmark = NULL,
  .get = NULL,
  .put = NULL,
  .marshal = NULL,
  .unmarshal = NULL,
  .tostring = NULL,
  .compare = NULL,
  .hash = NULL,
  .next = NULL,
  .call = NULL,
};

typedef struct {
  immer::set<Janet>::iterator actual;
  Janet backing_set;
} SetIterator;

static void check_set_iterator(void *data, SetIterator *iterator) {
  if (data != janet_unwrap_abstract(iterator->backing_set)) {
    janet_panicf("foreign iterator");
  }
}

static int set_iterator_gcmark(void *data, size_t len) {
  (void) len;
  auto iterator = CAST_SET_ITERATOR(data);
  janet_mark(iterator->backing_set);
  return 0;
}

static Janet set_iterator_next(void *data, Janet key);
static int set_iterator_get(void *data, Janet key, Janet *out);
static const JanetAbstractType set_iterator_type = {
  "jimmy/set-iterator",
  .gc = NULL,
  .gcmark = set_iterator_gcmark,
  .get = set_iterator_get,
  .put = NULL,
  .marshal = NULL,
  .unmarshal = NULL,
  .tostring = NULL,
  .compare = NULL,
  .hash = NULL,
  .next = set_iterator_next,
  .call = NULL,
};

static Janet set_iterator_next(void *data, Janet key) {
  if (janet_checktype(key, JANET_NIL)) {
    return janet_wrap_abstract(data);
  } else if (janet_checkabstract(key, &set_iterator_type) && janet_unwrap_abstract(key) == data) {
    auto iterator = CAST_SET_ITERATOR(janet_unwrap_abstract(key));
    auto set = CAST_SET(janet_unwrap_abstract(iterator->backing_set));
    iterator->actual++;
    if (iterator->actual == set->end()) {
      return janet_wrap_nil();
    } else {
      return key;
    }
  } else {
    janet_panicf("illegal key %v", key);
  }
}

static int set_iterator_get(void *data, Janet key, Janet *out) {
  if (janet_checkabstract(key, &set_iterator_type) && janet_unwrap_abstract(key) == data) {
    auto iterator = CAST_SET_ITERATOR(janet_unwrap_abstract(key));
    auto set = CAST_SET(janet_unwrap_abstract(iterator->backing_set));
    if (iterator->actual == set->end()) {
      return 0;
    } else {
      *out = *iterator->actual;
      return 1;
    }
  } else {
    return 0;
  }
}

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

static Janet cfun_set_union(int32_t argc, Janet *argv);
static Janet cfun_set_difference(int32_t argc, Janet *argv);
static Janet cfun_set_intersection(int32_t argc, Janet *argv);
static const JanetMethod set_methods[] = {
  {"+", cfun_set_union},
  {"-", cfun_set_difference},
  {"*", cfun_set_intersection},
  {"length", cfun_set_length},
  {NULL, NULL}
};

static int set_get(void *data, Janet key, Janet *out) {
  if (janet_checkabstract(key, &set_iterator_type)) {
    auto iterator = CAST_SET_ITERATOR(janet_unwrap_abstract(key));
    check_set_iterator(data, iterator);
    *out = *iterator->actual;
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
    auto iterator = CAST_SET_ITERATOR(janet_abstract(&set_iterator_type, sizeof(SetIterator)));
    iterator->backing_set = janet_wrap_abstract(data);
    iterator->actual = set->begin();
    return janet_wrap_abstract(iterator);
  }

  if (!janet_checkabstract(key, &set_iterator_type)) {
    janet_panicf("set key should be an iterator; got %v", key);
  }
  auto iterator = CAST_SET_ITERATOR(janet_unwrap_abstract(key));
  check_set_iterator(data, iterator);
  iterator->actual++;
  if (iterator->actual == set->end()) {
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

static int32_t set_hash(void *data, size_t len) {
  (void) len;
  auto set = CAST_SET(data);
  // start with a random permutation of 16 1s and 16 0s
  uint32_t hash = 0b01111110101101101101010000000001;
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

static Janet cfun_set_of(int32_t argc, Janet *argv) {
  janet_fixarity(argc, 1);
  Janet iterable = argv[0];
  auto set = NEW_SET();
  auto transient = NEW_TSET();
  *transient = set->transient();

  auto key = janet_wrap_nil();
  while (true) {
    key = janet_next(iterable, key);
    if (janet_checktype(key, JANET_NIL)) {
      break;
    }
    transient->insert(janet_in(iterable, key));
  }

  *set = transient->persistent();
  return janet_wrap_abstract(set);
}

static Janet cfun_set_of_keys(int32_t argc, Janet *argv) {
  janet_fixarity(argc, 1);
  Janet iterable = argv[0];
  auto set = NEW_SET();
  auto transient = NEW_TSET();
  *transient = set->transient();

  auto key = janet_wrap_nil();
  while (true) {
    key = janet_next(iterable, key);
    if (janet_checktype(key, JANET_NIL)) {
      break;
    }
    transient->insert(key);
  }

  *set = transient->persistent();
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
  auto transient = NEW_TSET();
  *transient = new_set->transient();
  for (int32_t i = 0; i < argc; i++) {
    auto old_set = CAST_SET(janet_getabstract(argv, i, &set_type));
    for (auto el : *old_set) {
      transient->insert(el);
    }
  }
  *new_set = transient->persistent();
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
  *new_set = transient.persistent();
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
  auto first_set = CAST_SET(janet_getabstract(argv, 0, &set_type));
  auto transient = NEW_TSET();
  *transient = first_set->transient();
  for (int32_t i = 1; i < argc; i++) {
    auto other_set = CAST_SET(janet_getabstract(argv, i, &set_type));
    for (auto el : *other_set) {
      transient->erase(el);
    }
  }
  auto new_set = NEW_SET();
  *new_set = transient->persistent();
  return janet_wrap_abstract(new_set);
}

static bool subset_helper(immer::set<Janet> *a, immer::set<Janet> *b, bool strict) {
  auto a_size = a->size();
  auto b_size = b->size();
  if (a_size > b_size) {
    return false;
  }
  if (a_size == b_size && strict) {
    return false;
  }
  for (auto el : *a) {
    if (!b->count(el)) {
      return false;
    }
  }
  return true;
}

static Janet cfun_set_subset(int32_t argc, Janet *argv) {
  janet_fixarity(argc, 2);
  return janet_wrap_boolean(subset_helper(
    CAST_SET(janet_getabstract(argv, 0, &set_type)),
    CAST_SET(janet_getabstract(argv, 1, &set_type)),
    false
  ));
}

static Janet cfun_set_superset(int32_t argc, Janet *argv) {
  janet_fixarity(argc, 2);
  return janet_wrap_boolean(subset_helper(
    CAST_SET(janet_getabstract(argv, 1, &set_type)),
    CAST_SET(janet_getabstract(argv, 0, &set_type)),
    false
  ));
}

static Janet cfun_set_strict_subset(int32_t argc, Janet *argv) {
  janet_fixarity(argc, 2);
  return janet_wrap_boolean(subset_helper(
    CAST_SET(janet_getabstract(argv, 0, &set_type)),
    CAST_SET(janet_getabstract(argv, 1, &set_type)),
    true
  ));
}

static Janet cfun_set_strict_superset(int32_t argc, Janet *argv) {
  janet_fixarity(argc, 2);
  return janet_wrap_boolean(subset_helper(
    CAST_SET(janet_getabstract(argv, 1, &set_type)),
    CAST_SET(janet_getabstract(argv, 0, &set_type)),
    true
  ));
}

static Janet cfun_set_to_tuple(int32_t argc, Janet *argv) {
  janet_fixarity(argc, 1);
  auto set = CAST_SET(janet_getabstract(argv, 0, &set_type));
  Janet *result = janet_tuple_begin(set->size());
  size_t i = 0;
  for (auto el : *set) {
    result[i++] = el;
  }
  return janet_wrap_tuple(janet_tuple_end(result));
}

static Janet cfun_set_to_array(int32_t argc, Janet *argv) {
  janet_fixarity(argc, 1);
  auto set = CAST_SET(janet_getabstract(argv, 0, &set_type));
  JanetArray *result = janet_array(set->size());
  for (auto el : *set) {
    janet_array_push(result, el);
  }
  return janet_wrap_array(result);
}

static Janet cfun_set_filter(int32_t argc, Janet *argv) {
  janet_fixarity(argc, 2);
  auto set = CAST_SET(janet_getabstract(argv, 0, &set_type));
  auto pred = argv[1];

  auto new_set = NEW_SET();
  auto transient = NEW_TSET();
  *transient = new_set->transient();
  for (auto el : *set) {
    if (janet_truthy(call_callable(pred, 1, &el))) {
      transient->insert(el);
    }
  }
  *new_set = transient->persistent();
  return janet_wrap_abstract(new_set);
}

static Janet cfun_set_map(int32_t argc, Janet *argv) {
  janet_fixarity(argc, 2);
  auto set = CAST_SET(janet_getabstract(argv, 0, &set_type));
  auto f = argv[1];

  auto new_set = NEW_SET();
  auto transient = NEW_TSET();
  *transient = new_set->transient();
  for (auto el : *set) {
    transient->insert(call_callable(f, 1, &el));
  }
  *new_set = transient->persistent();
  return janet_wrap_abstract(new_set);
}

static Janet cfun_set_filter_map(int32_t argc, Janet *argv) {
  janet_fixarity(argc, 2);
  auto set = CAST_SET(janet_getabstract(argv, 0, &set_type));
  auto f = argv[1];

  auto new_set = NEW_SET();
  auto transient = NEW_TSET();
  *transient = new_set->transient();
  for (auto el : *set) {
    Janet x = call_callable(f, 1, &el);
    if (!janet_checktype(x, JANET_NIL)) {
      transient->insert(x);
    }
  }
  *new_set = transient->persistent();
  return janet_wrap_abstract(new_set);
}

static Janet cfun_set_reduce(int32_t argc, Janet *argv) {
  janet_fixarity(argc, 3);
  auto set = CAST_SET(janet_getabstract(argv, 0, &set_type));
  auto acc = argv[1];
  auto f = argv[2];

  for (auto el : *set) {
    Janet args[2] = { acc, el };
    acc = call_callable(f, 2, args);
  }
  return acc;
}

static Janet cfun_set_count(int32_t argc, Janet *argv) {
  janet_fixarity(argc, 2);
  auto set = CAST_SET(janet_getabstract(argv, 0, &set_type));
  auto pred = argv[1];
  int32_t count = 0;
  for (auto el : *set) {
    if (janet_truthy(call_callable(pred, 1, &el))) {
      count++;
    }
  }
  return janet_wrap_integer(count);
}

static const JanetReg set_cfuns[] = {
  {"set/new", cfun_set_new, "(set/new & xs)\n\n"
    "Returns a persistent immutable set containing only the listed elements."},
  {"set/of", cfun_set_of, "(set/of iterable)\n\n"
    "Returns a set of all the values in an iterable data structure."},
  {"set/of-keys", cfun_set_of_keys, "(set/of-keys iterable)\n\n"
    "Returns a set of all the keys in an interable data structure."},
  {"set/add", cfun_set_add, "(set/add set & xs)\n\n"
    "Returns a new set containing all of the elements from the original set and all of the subsequent arguments."},
  {"set/remove", cfun_set_remove, "(set/remove set & xs)\n\n"
    "Returns a new set containing all of the elements from the original set except any of the subsequent arguments."},
  {"set/union", cfun_set_union, "(set/union & sets)\n\n"
    "Returns a set that is the union of all of its arguments."},
  {"set/intersection", cfun_set_intersection, "(set/intersection & sets)\n\n"
    "Returns a set that is the intersection of all of its arguments. Naively folds when given more than two arguments."},
  {"set/difference", cfun_set_difference, "(set/difference set & sets)\n\n"
    "Returns a set that is the first set minus all of the latter sets."},
  {"set/subset?", cfun_set_subset, "(set/subset? a b)\n\n"
    "Returns true if `a` is a subset of `b`."},
  {"set/superset?", cfun_set_superset, "(set/superset? a b)\n\n"
    "Returns true if `a` is a superset of `b`."},
  {"set/strict-subset?", cfun_set_strict_subset, "(set/strict-subset? a b)\n\n"
    "Returns true if `a` is a strict subset of `b`."},
  {"set/strict-superset?", cfun_set_strict_superset, "(set/strict-superset? a b)\n\n"
    "Returns true if `a` is a strict superset of `b`."},
  {"set/to-tuple", cfun_set_to_tuple, "(set/to-tuple set)\n\n"
    "Returns a tuple of all of the elements in the set, in no particular order."},
  {"set/to-array", cfun_set_to_array, "(set/to-array set)\n\n"
    "Returns an array of all of the elements in the set, in no particular order."},
  {"set/map", cfun_set_map, "(set/map set f)\n\n"
    "Returns a new set derived from the given transformation function. "
    "`f` can be any callable value, not just a function.\n\n"
    "Note that the arguments are in the opposite order of Janet's `map` function."},
  {"set/filter", cfun_set_filter, "(set/filter set pred)\n\n"
    "Returns a set containing only the elements for which the predicate returns a truthy value. "
    "`pred` can be any callable value, not just a function.\n\n"
    "Note that the arguments are in the opposite order of Janet's `filter` function."},
  {"set/reduce", cfun_set_reduce, "(set/reduce set init f)\n\n"
    "Returns a reduction of the elements in the set, which will be traversed in arbitrary order. "
    "`f` can be any callable value, not just a function.\n\n"
    "Note that the arguments are in a different order than Janet's `reduce` function."},
  {"set/count", cfun_set_count, "(set/count set pred)\n\n"
    "Returns the number of elements in the set that match the given predicate. "
    "`pred` can be any callable value, not just a function.\n\n"
    "Note that the arguments are in a different order than Janet's `count` function."},
  {"set/filter-map", cfun_set_filter_map, "(set/filter-map set f)\n\n"
    "Like `set/map`, but excludes `nil`. "
    "`f` can be any callable value, not just a function."},
  {NULL, NULL, NULL}
};
