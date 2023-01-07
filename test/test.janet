(import /set)

(defn assert-equal [a b]
  (assert (= a b))
  (assert (= (hash a) (hash b))))

(defmacro assert-throws [expr err]
  ~(try ,expr ([err] (assert (= err ,err)))))

(assert-equal
  (set/add (set/new 1 2) 3)
  (set/remove (set/new 1 2 3 4) 4))

(def x (set/new 1 2 3))
(def y (set/new 1 2 3))
(assert-equal x y)
(set/add x 4 5 6)
(set/remove y 1 2 3)
(assert-equal x y)

(assert-equal (set/add x 4 5 6) (set/new 1 2 3 4 5 6))
(assert-equal (set/remove x 1 2 3) (set/new))
(assert-equal (set/new) set/empty)

(assert (= 1 (length (set/new nil nil nil))))

(assert-equal (set/union (set/new 1 2 3) (set/new 2 3 4))
              (set/union (set/new 1) (set/new 2) (set/new 3) (set/new 4)))
(assert-equal (set/union) set/empty)
(assert-throws (set/union (set/new 1) 2) "bad slot #1, expected jimmy/set, got 2")

(assert-equal (set/intersection (set/new 1 2 3) (set/new 2 3 4))
              (set/intersection (set/new 1 2 3 4 5) (set/new 2 3) (set/new 1 2 3 4 5 6)))
(assert-equal (set/intersection (set/new 1 2 3) (set/new 4 5 6)) set/empty)
(assert-throws (set/intersection (set/new 1) 2) "bad slot #1, expected jimmy/set, got 2")

(assert-equal (set/difference (set/new 1 2 3 4 5) (set/new 2 3))
              (set/difference (set/new 1 4 5 6 7) (set/new 6) (set/new 7)))
(assert-equal (set/difference (set/new 1 2 3) (set/new 1 2 3)) set/empty)

(assert-throws (set/difference 1 2) "bad slot #0, expected jimmy/set, got 1")
(assert-throws (set/difference (set/new 1) 2) "bad slot #1, expected jimmy/set, got 2")
