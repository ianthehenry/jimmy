(defn unprefixed [name prefix]
  (if (string/has-prefix? prefix name)
    (symbol (string/slice name (length prefix)))))

(defn export-prefix [module prefix]
  (def env (curenv))
  (eachp [binding value] (require module)
    (if-let [name (unprefixed binding prefix)]
      (put env name value))))
