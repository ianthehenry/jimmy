# Our native module contains bindings for sets and maps (in theory), but we want
# to be able to (import jimmy/set) and get just the set-related functions.
(let [env (curenv)
      unprefixed
        (fn [name prefix]
          (if (string/has-prefix? prefix name)
            (symbol (string/slice name (length prefix)))))]
  (eachp [binding value] (require "jimmy/native")
    (if-let [name (unprefixed binding "set/")]
      (put env name value))))

(def empty (new))
