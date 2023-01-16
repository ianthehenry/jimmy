#include <immer/map.hpp>
#include <immer/map_transient.hpp>

#define CAST_MAP(expr) static_cast<immer::map<Janet, Janet> *>((expr))
#define CAST_MAP_ITERATOR(expr) static_cast<MapIterator *>((expr))
#define NEW_MAP() new (janet_abstract(&map_type, sizeof(immer::map<Janet, Janet>))) immer::map<Janet, Janet>()
#define KVP(k, v) std::pair<Janet, Janet>((k), (v))

typedef enum {
  Keys,
  Values,
  Pairs,
} IteratorType;

typedef struct {
  immer::map<Janet, Janet>::iterator actual;
  Janet backing_map;
  IteratorType type;
} MapIterator;

static Janet map_iterator_value(MapIterator *iterator) {
  std::pair<Janet, Janet> pair = *iterator->actual;
  switch (iterator->type) {
  case Keys: return pair.first;
  case Values: return pair.second;
  case Pairs: return pair_to_tuple(pair);
  }
}

static void check_map_iterator(void *data, MapIterator *iterator) {
  if (data != janet_unwrap_abstract(iterator->backing_map)) {
    janet_panicf("foreign iterator");
  }
}

static int map_iterator_gcmark(void *data, size_t len) {
  (void) len;
  auto iterator = CAST_MAP_ITERATOR(data);
  janet_mark(iterator->backing_map);
  return 0;
}

static Janet map_iterator_next(void *data, Janet key);
static int map_iterator_get(void *data, Janet key, Janet *out);
static const JanetAbstractType map_iterator_type = {
  "jimmy/map-iterator",
  .gc = NULL,
  .gcmark = map_iterator_gcmark,
  .get = map_iterator_get,
  .put = NULL,
  .marshal = NULL,
  .unmarshal = NULL,
  .tostring = NULL,
  .compare = NULL,
  .hash = NULL,
  .next = map_iterator_next,
  .call = NULL,
};

static Janet map_iterator_next(void *data, Janet key) {
  if (janet_checktype(key, JANET_NIL)) {
    return janet_wrap_abstract(data);
  } else if (janet_checkabstract(key, &map_iterator_type) && janet_unwrap_abstract(key) == data) {
    auto iterator = CAST_MAP_ITERATOR(janet_unwrap_abstract(key));
    auto map = CAST_MAP(janet_unwrap_abstract(iterator->backing_map));
    iterator->actual++;
    if (iterator->actual == map->end()) {
      return janet_wrap_nil();
    } else {
      return key;
    }
  } else {
    janet_panicf("illegal key %v", key);
  }
}

static int map_iterator_get(void *data, Janet key, Janet *out) {
  if (janet_checkabstract(key, &map_iterator_type) && janet_unwrap_abstract(key) == data) {
    auto iterator = CAST_MAP_ITERATOR(janet_unwrap_abstract(key));
    auto map = CAST_MAP(janet_unwrap_abstract(iterator->backing_map));
    if (iterator->actual == map->end()) {
      return 0;
    } else {
      *out = map_iterator_value(iterator);
      return 1;
    }
  } else {
    return 0;
  }
}

static int map_gc(void *data, size_t len) {
  (void) len;
  auto map = CAST_MAP(data);
  map->~map();
  return 0;
}

static int map_gcmark(void *data, size_t len) {
  (void) len;
  auto map = CAST_MAP(data);
  for (auto pair : *map) {
    janet_mark(pair.first);
    janet_mark(pair.second);
  }
  return 0;
}

static void map_tostring(void *data, JanetBuffer *buffer) {
  auto map = CAST_MAP(data);
  janet_buffer_push_cstring(buffer, "{");
  int first = 1;
  for (auto pair : *map) {
    if (first) {
      first = 0;
    } else {
      janet_buffer_push_cstring(buffer, " ");
    }
    janet_pretty(buffer, 0, 0, pair.first);
    janet_buffer_push_cstring(buffer, " ");
    janet_pretty(buffer, 0, 0, pair.second);
  }
  janet_buffer_push_cstring(buffer, "}");
}

static Janet cfun_map_length(int32_t argc, Janet *argv) {
  janet_fixarity(argc, 1);
  auto map = CAST_MAP(janet_unwrap_abstract(argv[0]));
  return janet_wrap_integer(static_cast<int32_t>(map->size()));
}

static const JanetMethod map_methods[] = {
  {"length", cfun_map_length},
  {NULL, NULL}
};

static int map_get(void *data, Janet key, Janet *out) {
  if (janet_checkabstract(key, &map_iterator_type)) {
    auto iterator = CAST_MAP_ITERATOR(janet_unwrap_abstract(key));
    check_map_iterator(data, iterator);
    *out = map_iterator_value(iterator);
    return 1;
  } else if (janet_checktype(key, JANET_KEYWORD)) {
    return janet_getmethod(janet_unwrap_keyword(key), map_methods, out);
  } else {
    return 0;
  }
}

static Janet new_map_iterator(immer::map<Janet, Janet> *map, IteratorType type) {
  auto iterator = CAST_MAP_ITERATOR(janet_abstract(&map_iterator_type, sizeof(MapIterator)));
  iterator->backing_map = janet_wrap_abstract(map);
  iterator->actual = map->begin();
  iterator->type = type;
  return janet_wrap_abstract(iterator);
}

static Janet map_next(void *data, Janet key) {
  auto map = CAST_MAP(data);

  if (janet_checktype(key, JANET_NIL)) {
    return new_map_iterator(map, IteratorType::Pairs);
  }

  if (!janet_checkabstract(key, &map_iterator_type)) {
    janet_panicf("map key should be an iterator; got %v", key);
  }
  auto iterator = CAST_MAP_ITERATOR(janet_unwrap_abstract(key));
  check_map_iterator(data, iterator);
  iterator->actual++;
  if (iterator->actual == map->end()) {
    return janet_wrap_nil();
  } else {
    return key;
  }
}

static Janet map_call(void *data, int32_t argc, Janet *argv) {
  janet_arity(argc, 1, 2);
  auto map = CAST_MAP(data);
  Janet key = argv[0];
  const Janet *result = map->find(key);
  if (result == NULL) {
    if (argc == 2) {
      // default value
      return argv[1];
    } else {
      janet_panicf("key %v not found", key);
    }
  } else {
    return *result;
  }
}

static void map_marshal(void *data, JanetMarshalContext *ctx) {
  janet_marshal_abstract(ctx, data);
  auto map = CAST_MAP(data);
  janet_marshal_int(ctx, static_cast<int32_t>(map->size()));
  for (auto pair : *map) {
    janet_marshal_janet(ctx, pair.first);
    janet_marshal_janet(ctx, pair.second);
  }
}

static void *map_unmarshal(JanetMarshalContext *ctx) {
  auto map = CAST_MAP(janet_unmarshal_abstract(ctx, sizeof(immer::map<Janet, Janet>)));
  new (map) immer::map<Janet, Janet>();
  auto transient = map->transient();
  int32_t size = janet_unmarshal_int(ctx);
  for (int32_t i = 0; i < size; i++) {
    auto key = janet_unmarshal_janet(ctx);
    auto value = janet_unmarshal_janet(ctx);
    transient.insert(KVP(key, value));
  }
  *map = transient.persistent();
  return map;
}

static int map_compare(void *data1, void *data2) {
  auto map1 = CAST_MAP(data1);
  auto map2 = CAST_MAP(data2);
  if (*map1 == *map2) {
    return 0;
  }
  return map1 > map2 ? 1 : -1;
}

static int32_t map_hash(void *data, size_t len) {
  (void) len;
  auto map = CAST_MAP(data);
  // start with a random permutation of 16 1s and 16 0s
  uint32_t hash = 0b11100110010111010100001111000001;
  // Iteration is order is stable over hash values, regardless
  // of how you construct the map.
  // https://github.com/arximboldi/immer/discussions/249#discussioncomment-4620734
  for (auto pair : *map) {
    hash = hash_mix(hash, static_cast<int32_t>(std::hash<Janet>()(pair.first)));
    hash = hash_mix(hash, static_cast<int32_t>(std::hash<Janet>()(pair.second)));
  }
  return hash;
}

static const JanetAbstractType map_type = {
  .name = "jimmy/map",
  .gc = map_gc,
  .gcmark = map_gcmark,
  .get = map_get,
  .put = NULL,
  .marshal = map_marshal,
  .unmarshal = map_unmarshal,
  .tostring = map_tostring,
  .compare = map_compare,
  .hash = map_hash,
  .next = map_next,
  .call = map_call,
};

static Janet cfun_map_new(int32_t argc, Janet *argv) {
  if (argc % 2 == 1) {
    janet_panic("expected even number of arguments");
  }
  auto map = NEW_MAP();
  auto transient = map->transient();
  for (int32_t i = 0; i < argc; i += 2) {
    transient.insert(KVP(argv[i + 0], argv[i + 1]));
  }
  *map = transient.persistent();
  return janet_wrap_abstract(map);
}

static Janet cfun_map_keys(int32_t argc, Janet *argv) {
  janet_fixarity(argc, 1);
  return new_map_iterator(CAST_MAP(janet_getabstract(argv, 0, &map_type)), IteratorType::Keys);
}

static Janet cfun_map_values(int32_t argc, Janet *argv) {
  janet_fixarity(argc, 1);
  return new_map_iterator(CAST_MAP(janet_getabstract(argv, 0, &map_type)), IteratorType::Values);
}

static Janet cfun_map_pairs(int32_t argc, Janet *argv) {
  janet_fixarity(argc, 1);
  return new_map_iterator(CAST_MAP(janet_getabstract(argv, 0, &map_type)), IteratorType::Pairs);
}

static const JanetReg map_cfuns[] = {
  {"map/new", cfun_map_new, "(map/new & kvs)\n\n"
    "Returns a persistent immutable map containing the listed entries."},
  {"map/keys", cfun_map_keys, "(map/keys map)\n\n"
    "Returns an iterator over the keys in the map."},
  {"map/values", cfun_map_values, "(map/values map)\n\n"
    "Returns an iterator over the values in the map."},
  {"map/pairs", cfun_map_pairs, "(map/pairs map)\n\n"
    "Returns an iterator over the key-value pairs in the map."},
  {NULL, NULL, NULL}
};
