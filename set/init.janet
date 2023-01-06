(let [env (curenv)
      unprefixed
        (fn [name prefix]
          (if (string/has-prefix? prefix name)
            (symbol (string/slice name (length prefix)))))]
  (eachp [binding value] (require "jimmy/native")
    (if-let [name (unprefixed binding "set/")]
      (put env name value))))
