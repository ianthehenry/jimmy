(defn assert= [a b]
  (unless (= a b)
    (error (string/format "%q != %q" a b)))
  (unless (= (hash a) (hash b))
    (error (string/format "(hash %q) != (hash %q)" a b))))

(defn assert-not= [a b]
  (when (= a b)
    (error (string/format "%q = %q" a b)))
  (when (= (hash a) (hash b))
    (error (string/format "(hash %q) = (hash %q)" a b))))

(defmacro assert-throws [expr err]
  ~(try (do ,expr (error "did not throw")) ([err] (assert= err ,err))))

(defmacro assert-round-trip [expr]
  (assert= expr (unmarshal (marshal expr))))
