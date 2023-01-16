(declare-project :name "jimmy")

(declare-native
  :name "jimmy/native"
  :source ["src/jimmy.cpp"]
  :cppflags ["-Iimmer" "-std=c++14"])

(declare-source
  :source [
    "src/set.janet"
    "src/init.janet"
  ]
  :prefix "jimmy")
