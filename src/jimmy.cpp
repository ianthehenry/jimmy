#include <janet.h>

#include "set.cpp"

JANET_MODULE_ENTRY(JanetTable *env) {
  janet_cfuns(env, "jimmy", set_cfuns);
  janet_register_abstract_type(&set_type);
  janet_register_abstract_type(&set_iterator_type);
}
