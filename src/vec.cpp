#include <immer/vector.hpp>
#include <immer/vector_transient.hpp>

#define CAST_VEC(expr) static_cast<immer::vector<Janet> *>((expr))
#define NEW_VEC() new (janet_abstract(&vec_type, sizeof(immer::vector<Janet>))) immer::vector<Janet>()

#define CAST_TVEC(expr) static_cast<immer::vector_transient<Janet> *>((expr))
#define NEW_TVEC() new (janet_abstract(&tvec_type, sizeof(immer::vector_transient<Janet>))) immer::vector_transient<Janet>()

static int tvec_gc(void *data, size_t len) {
  (void) len;
  auto tvec = CAST_TVEC(data);
  tvec->~vector_transient();
  return 0;
}

// The tvec abstract type is not exposed to the user. Its only use is to properly deallocate even when there is a panic.
static const JanetAbstractType tvec_type = {
  .name = "jimmy/tvec",
  .gc = tvec_gc,
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

static int vec_gc(void *data, size_t len) {
  (void) len;
  auto vec = CAST_VEC(data);
  vec->~vector();
  return 0;
}

static int vec_gcmark(void *data, size_t len) {
  (void) len;
  auto vec = CAST_VEC(data);
  for (auto el : *vec) {
    janet_mark(el);
  }
  return 0;
}

static void vec_tostring(void *data, JanetBuffer *buffer) {
  auto vec = CAST_VEC(data);
  janet_buffer_push_cstring(buffer, "[");
  int first = 1;
  for (auto el : *vec) {
    if (first) {
      first = 0;
    } else {
      janet_buffer_push_cstring(buffer, " ");
    }
    janet_pretty(buffer, 0, 0, el);
  }
  janet_buffer_push_cstring(buffer, "]");
}

static Janet cfun_vec_length(int32_t argc, Janet *argv) {
  janet_fixarity(argc, 1);
  auto vec = CAST_VEC(janet_unwrap_abstract(argv[0]));
  return janet_wrap_number(static_cast<double>(vec->size()));
}

static const JanetMethod vec_methods[] = {
  {"length", cfun_vec_length},
  {NULL, NULL}
};

static int vec_get(void *data, Janet key, Janet *out) {
  if (janet_checksize(key)) {
    size_t index = static_cast<size_t>(janet_unwrap_number(key));
    auto vec = CAST_VEC(data);
    if (index >= vec->size()) {
      return 0;
    }
    *out = (*vec)[index];
    return 1;
  } else if (janet_checktype(key, JANET_KEYWORD)) {
    return janet_getmethod(janet_unwrap_keyword(key), vec_methods, out);
  } else {
    return 0;
  }
}

static Janet vec_next(void *data, Janet key) {
  auto vec = CAST_VEC(data);
  if (janet_checktype(key, JANET_NIL)) {
    if (!vec->empty()) {
      return janet_wrap_integer(0);
    }
  } else if (janet_checksize(key)) {
    size_t index = janet_unwrap_number(key);
    if (index+1 < vec->size()) {
      return janet_wrap_integer(index + 1);
    }
  }
  return janet_wrap_nil();
}

static Janet vec_call(void *data, int32_t argc, Janet *argv) {
  janet_fixarity(argc, 1);
  size_t index = janet_getsize(argv, 0);
  auto vec = CAST_VEC(data);
  if (index >= vec->size()) {
    janet_panicf("expected integer key in range [0, %d), got %v", vec->size(), argv[0]);
  }
  return (*vec)[index];
}

static void vec_marshal(void *data, JanetMarshalContext *ctx) {
  janet_marshal_abstract(ctx, data);
  auto vec = CAST_VEC(data);
  janet_marshal_size(ctx, vec->size());
  for (auto el : *vec) {
    janet_marshal_janet(ctx, el);
  }
}

static void *vec_unmarshal(JanetMarshalContext *ctx) {
  auto vec = CAST_VEC(janet_unmarshal_abstract(ctx, sizeof(immer::vector<Janet>)));
  new (vec) immer::vector<Janet>();
  auto transient = vec->transient();
  size_t size = janet_unmarshal_size(ctx);
  for (size_t i = 0; i < size; i++) {
    transient.push_back(janet_unmarshal_janet(ctx));
  }
  *vec = transient.persistent();
  return vec;
}

static int vec_compare(void *data1, void *data2) {
  auto vec1 = CAST_VEC(data1);
  auto vec2 = CAST_VEC(data2);
  if (*vec1 == *vec2) {
    return 0;
  }
  return vec1 > vec2 ? 1 : -1;
}

static int32_t vec_hash(void *data, size_t len) {
  (void) len;
  auto vec = CAST_VEC(data);
  // start with a random value
  uint32_t hash = 0x6bd33241;
  for (auto el : *vec) {
    hash = hash_mix(hash, static_cast<int32_t>(std::hash<Janet>()(el)));
  }
  return hash;
}

static const JanetAbstractType vec_type = {
  .name = "jimmy/vec",
  .gc = vec_gc,
  .gcmark = vec_gcmark,
  .get = vec_get,
  .put = NULL,
  .marshal = vec_marshal,
  .unmarshal = vec_unmarshal,
  .tostring = vec_tostring,
  .compare = vec_compare,
  .hash = vec_hash,
  .next = vec_next,
  .call = vec_call,
};

static Janet cfun_vec_new(int32_t argc, Janet *argv) {
  auto vec = NEW_VEC();
  auto transient = vec->transient();
  for (int32_t i = 0; i < argc; i++) {
    transient.push_back(argv[i]);
  }
  *vec = transient.persistent();
  return janet_wrap_abstract(vec);
}

static Janet cfun_vec_of(int32_t argc, Janet *argv) {
  janet_fixarity(argc, 1);
  Janet iterable = argv[0];
  auto vec = NEW_VEC();
  auto transient = NEW_TVEC();
  *transient = vec->transient();

  Janet key = janet_wrap_nil();
  while (true) {
    key = janet_next(iterable, key);
    if (janet_checktype(key, JANET_NIL)) {
      break;
    }
    transient->push_back(janet_in(iterable, key));
  }

  *vec = transient->persistent();
  return janet_wrap_abstract(vec);
}

static Janet cfun_vec_push(int32_t argc, Janet *argv) {
  janet_arity(argc, 1, -1);
  auto old_vec = CAST_VEC(janet_getabstract(argv, 0, &vec_type));
  auto new_vec = NEW_VEC();

  int32_t new_elements = argc - 1;
  if (new_elements == 1) {
    *new_vec = old_vec->push_back(argv[1]);
  } else {
    auto transient = old_vec->transient();
    for (int32_t i = 1; i < argc; i++) {
      transient.push_back(argv[i]);
    }
    *new_vec = transient.persistent();
  }
  return janet_wrap_abstract(new_vec);
}

static Janet cfun_vec_take(int32_t argc, Janet *argv) {
  janet_fixarity(argc, 2);
  size_t n = janet_getsize(argv, 1);
  auto old_vec = CAST_VEC(janet_getabstract(argv, 0, &vec_type));
  auto new_vec = NEW_VEC();
  *new_vec = old_vec->take(n);
  return janet_wrap_abstract(new_vec);
}

static Janet cfun_vec_pop(int32_t argc, Janet *argv) {
  janet_fixarity(argc, 1);
  auto old_vec = CAST_VEC(janet_getabstract(argv, 0, &vec_type));
  if (old_vec->empty()) {
    return argv[0];
  }
  auto new_vec = NEW_VEC();
  *new_vec = old_vec->take(old_vec->size() - 1);
  return janet_wrap_abstract(new_vec);
}

static Janet cfun_vec_popn(int32_t argc, Janet *argv) {
  janet_fixarity(argc, 2);
  size_t n = janet_getsize(argv, 1);
  auto old_vec = CAST_VEC(janet_getabstract(argv, 0, &vec_type));
  auto new_vec = NEW_VEC();
  if (n < old_vec->size()) {
    *new_vec = old_vec->take(old_vec->size() - n);
  }
  return janet_wrap_abstract(new_vec);
}

static Janet cfun_vec_put(int32_t argc, Janet *argv) {
  janet_fixarity(argc, 3);
  size_t n = janet_getsize(argv, 1);
  auto old_vec = CAST_VEC(janet_getabstract(argv, 0, &vec_type));
  if (n >= old_vec->size()) {
    janet_panicf("expected integer key in range [0, %d), got %v", old_vec->size(), argv[1]);
  }
  auto new_vec = NEW_VEC();
  *new_vec = old_vec->set(n, argv[2]);
  return janet_wrap_abstract(new_vec);
}

static Janet cfun_vec_first(int32_t argc, Janet *argv) {
  janet_fixarity(argc, 1);
  auto vec = CAST_VEC(janet_getabstract(argv, 0, &vec_type));
  if (vec->empty()) {
    return janet_wrap_nil();
  }
  return vec->front();
}

static Janet cfun_vec_last(int32_t argc, Janet *argv) {
  janet_fixarity(argc, 1);
  auto vec = CAST_VEC(janet_getabstract(argv, 0, &vec_type));
  if (vec->empty()) {
    return janet_wrap_nil();
  }
  return vec->back();
}

static Janet cfun_vec_to_tuple(int32_t argc, Janet *argv) {
  janet_fixarity(argc, 1);
  auto vec = CAST_VEC(janet_getabstract(argv, 0, &vec_type));
  Janet *result = janet_tuple_begin(vec->size());
  size_t i = 0;
  for (auto el : *vec) {
    result[i++] = el;
  }
  return janet_wrap_tuple(janet_tuple_end(result));
}

static Janet cfun_vec_to_array(int32_t argc, Janet *argv) {
  janet_fixarity(argc, 1);
  auto vec = CAST_VEC(janet_getabstract(argv, 0, &vec_type));
  JanetArray *result = janet_array(vec->size());
  for (auto el : *vec) {
    janet_array_push(result, el);
  }
  return janet_wrap_array(result);
}

static Janet cfun_vec_map(int32_t argc, Janet *argv) {
  janet_fixarity(argc, 2);
  auto vec = CAST_VEC(janet_getabstract(argv, 0, &vec_type));
  auto f = argv[1];

  auto new_vec = NEW_VEC();
  auto tvec = NEW_TVEC();
  *tvec = new_vec->transient();
  for (auto el : *vec) {
    tvec->push_back(call_callable(f, 1, &el));
  }
  *new_vec = tvec->persistent();
  return janet_wrap_abstract(new_vec);
}

static Janet cfun_vec_filter(int32_t argc, Janet *argv) {
  janet_fixarity(argc, 2);
  auto vec = CAST_VEC(janet_getabstract(argv, 0, &vec_type));
  auto f = argv[1];

  auto new_vec = NEW_VEC();
  auto tvec = NEW_TVEC();
  *tvec = new_vec->transient();
  for (auto el : *vec) {
    if (janet_truthy(call_callable(f, 1, &el))) {
      tvec->push_back(el);
    }
  }
  *new_vec = tvec->persistent();
  return janet_wrap_abstract(new_vec);
}

static Janet cfun_vec_filter_map(int32_t argc, Janet *argv) {
  janet_fixarity(argc, 2);
  auto vec = CAST_VEC(janet_getabstract(argv, 0, &vec_type));
  auto f = argv[1];

  auto new_vec = NEW_VEC();
  auto tvec = NEW_TVEC();
  *tvec = new_vec->transient();
  for (auto el : *vec) {
    Janet x = call_callable(f, 1, &el);
    if (!janet_checktype(x, JANET_NIL)) {
      tvec->push_back(x);
    }
  }
  *new_vec = tvec->persistent();
  return janet_wrap_abstract(new_vec);
}

static Janet cfun_vec_reduce(int32_t argc, Janet *argv) {
  janet_fixarity(argc, 3);
  auto vec = CAST_VEC(janet_getabstract(argv, 0, &vec_type));
  auto acc = argv[1];
  auto f = argv[2];

  for (auto el : *vec) {
    Janet args[2] = { acc, el };
    acc = call_callable(f, 2, args);
  }
  return acc;
}

static Janet cfun_vec_count(int32_t argc, Janet *argv) {
  janet_fixarity(argc, 2);
  auto vec = CAST_VEC(janet_getabstract(argv, 0, &vec_type));
  auto pred = argv[1];
  int32_t count = 0;
  for (auto el : *vec) {
    if (janet_truthy(call_callable(pred, 1, &el))) {
      count++;
    }
  }
  return janet_wrap_integer(count);
}

static const JanetReg vec_cfuns[] = {
  {"vec/new", cfun_vec_new, "(vec/new & xs)\n\n"
   "Returns a persistent immutable vector containing only the listed elements."},
  {"vec/of", cfun_vec_of, "(vec/of iterable)\n\n"
   "Returns a vector of all the values in an iterable data structure."},
  {"vec/push", cfun_vec_push, "(vec/push vec & xs)\n\n"
   "Returns a new vector containing all of the elements from the original and all of the subsequent arguments."},
  {"vec/take", cfun_vec_take, "(vec/take vec n)\n\n"
   "Returns a new vector containing the first n elements of vec, or all of vec if n >= (length vec)."},
  {"vec/pop", cfun_vec_pop, "(vec/pop vec)\n\n"
   "Returns a new vector with the last element removed."},
  {"vec/popn", cfun_vec_popn, "(vec/popn vec)\n\n"
   "Returns a new vector with the last n elements removed."},
  {"vec/put", cfun_vec_put, "(vec/put vec n val)\n\n"
   "Returns a new vector with nth element set to val."},
  {"vec/first", cfun_vec_first, "(vec/first vec)\n\n"
   "Returns the first element of the vector."},
  {"vec/last", cfun_vec_last, "(vec/last vec)\n\n"
   "Returns the last element of the vector."},
  {"vec/to-tuple", cfun_vec_to_tuple, "(vec/to-tuple vec)\n\n"
    "Returns a tuple of all of the elements in the vector."},
  {"vec/to-array", cfun_vec_to_array, "(vec/to-array vec)\n\n"
    "Returns an array of all of the elements in the vector."},
  {"vec/map", cfun_vec_map, "(vec/map vec f)\n\n"
   "Returns a new vector derived from the given transformation function. "
   "`f` can be any callable value, not just a function.\n\n"
   "Note that the arguments are in the opposite order of Janet's `map` function."},
  {"vec/filter", cfun_vec_filter, "(vec/filter vec pred)\n\n"
    "Returns a vector containing only the elements for which the predicate returns a truthy value. "
    "`pred` can be any callable value, not just a function.\n\n"
    "Note that the arguments are in the opposite order of Janet's `filter` function."},
  {"vec/reduce", cfun_vec_reduce, "(vec/reduce vec init f)\n\n"
    "Returns a reduction of the elements in the vector. `f` can be any callable value, not just a function.\n\n"
    "Note that the arguments are in a different order than Janet's `reduce` function."},
  {"vec/count", cfun_vec_count, "(vec/count vec pred)\n\n"
    "Returns the number of elements in the vector that match the given predicate. "
    "`pred` can be any callable value, not just a function.\n\n"
    "Note that the arguments are in a different order than Janet's `count` function."},
  {"vec/filter-map", cfun_vec_filter_map, "(vec/filter-map vec f)\n\n"
    "Like `vec/map`, but excludes `nil`. "
    "`f` can be any callable value, not just a function."},
  {NULL, NULL, NULL}
};
