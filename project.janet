(declare-project :name "jimmy")

(declare-native
  :name "jimmy/native"
  :source ["jimmy.cpp"]
  :cppflags ["-Iimmer" "-std=c++14"])

(declare-source
  :source "set/init.janet"
  :prefix "jimmy/set")
