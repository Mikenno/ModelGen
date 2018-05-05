

assert null == null

assert null != false
assert null != true

assert 2 == 2
assert 2 == 2.0
assert 2.0 == 2
assert 2.0 == 2.0

assert 2 != 3
assert 2 != 3.0
assert 2.0 != 3
assert 2.0 != 3.0

assert "test" == "test"
assert "test" != "test2"

assert () == ()
assert (1, 2, 3) == (1, 2, 3)
assert (1, 2, 3) != (1, 2, 4)
assert (1, 2, 3) != (1, 2, 3, 4)

assert [] == []
assert [1, 2, 3] == [1, 2, 3]
assert [1, 2, 3] != [1, 2, 4]
assert [1, 2, 3] != [1, 2, 3, 4]

assert [(1, 2), (3, 4)] == [(1, 2), (3, 4)]

assert {} == {}
assert {"a": 1} == {"a": 1}
assert {"a": 1} != {"a": 2}
assert {"a": 1} != {"b": 1}
assert {"a": 1} != {"a": 1, "b": 1}
assert {"a": 1} != {"a": 2, "b": 1}
assert {"a": 1, "b": 2, "c": 3} == {"c": 3, "b": 2, "a": 1}

assert print == print
assert print != range

func f1()
func f2()

assert f1 == f1
assert f1 != f2

proc p1()
proc p2()

assert p1 == p1
assert p1 != p2

assert (2 < 2, 2 < 3, 3 < 2) == (0, 1, 0)
assert (2 <= 2, 2 <= 3, 3 <= 2) == (1, 1, 0)

assert (2 > 2, 2 > 3, 3 > 2) == (0, 0, 1)
assert (2 >= 2, 2 >= 3, 3 >= 2) == (1, 0, 1)

assert "A" == "A"
assert "A" != "B"
assert "B" != "A"
assert "B" == "B"

assert ("A" < "A") == false
assert ("A" < "B") == true
assert ("B" < "A") == false
assert ("B" < "B") == false

assert ("A" > "A") == false
assert ("A" > "B") == false
assert ("B" > "A") == true
assert ("B" > "B") == false

assert ("A" <= "A") == true
assert ("A" <= "B") == true
assert ("B" <= "A") == false
assert ("B" <= "B") == true

assert ("A" >= "A") == true
assert ("A" >= "B") == false
assert ("B" >= "A") == true
assert ("B" >= "B") == true
