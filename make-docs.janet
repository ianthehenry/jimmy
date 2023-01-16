(def functions
  (seq [[name binding] :pairs (require "./build/jimmy/native")
        :when (table? binding) :when (binding :doc)]
    [name ;(string/split "\n\n" (binding :doc))]))

(var is-first true)
(each [name header body] (sort-by (fn [[name _ _]] name) functions)
  (unless is-first
    (print "\n---\n"))
  (set is-first false)
  (printf "```janet\n%s\n```\n\n%s" header body))
