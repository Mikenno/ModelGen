
import math


a = [0, 1, 2, 3, 4]
assert a == [0, 1, 2, 3, 4]
delete a[1]
assert a == [0, 2, 3, 4]
delete a[-2]
assert a == [0, 2, 4]


a = [1, 2]
a.add(3)
a.add(4)
a.add(5)
assert a == [1, 2, 3, 4, 5]

a.extend(range(6, 11))
assert a == [1, 2, 3, 4, 5, 6, 7, 8, 9, 10]

a = a.slice(0, 5)
assert a == [1, 2, 3, 4, 5]

a = map(a -> a - 1, a)
assert a == [0, 1, 2, 3, 4]

assert a.slice() == [0, 1, 2, 3, 4]
assert a.slice(2) == [2, 3, 4]
assert a.slice(-2) == [3, 4]
assert a.slice(1, -1) == [1, 2, 3]
assert a.slice(0, a.size, 2) == [0, 2, 4]
assert a.slice(1, a.size, 2) == [1, 3]


a = [[1], [2], [3], [4]]
b = a.copy()
c = a.slice(1, -1)

assert a == [[1], [2], [3], [4]]
assert b == [[1], [2], [3], [4]]
assert c == [[2], [3]]

a[0].add(5)
b[1].add(6)
c[1].add(7)

assert a == [[1, 5], [2, 6], [3, 7], [4]]
assert b == [[1, 5], [2, 6], [3, 7], [4]]
assert c == [[2, 6], [3, 7]]


a = []
a.extend(range(1, 7))

assert a == [1, 2, 3, 4, 5, 6]

e = math.even
a.sort((a, b) -> e(a) == e(b) ? a > b : (e(a) ? -1 : 0))

assert a == [1, 3, 5, 2, 4, 6]


a = range(8)
assert a == [0, 1, 2, 3, 4, 5, 6, 7]
assert a.pop() == 7
assert a.pop() == 6
assert a.pop(-2) == 4
assert a.pop(-2) == 3
assert a.pop(1) == 1
assert a == [0, 2, 5]
a.insert(1, 22)
a.insert(1, 11)
a.insert(3, 33)
a.insert(-1, 44)
a.insert(-1, 55)
assert a == [0, 11, 22, 33, 2, 44, 55, 5]
assert a.pop(0) == 0
assert a.pop(0) == 11
assert a.pop(0) == 22
assert a.pop(0) == 33
assert a.pop() == 5
assert a == [2, 44, 55]


a = [1, 2, 2, 3, 3, 3, 4, 4, 4, 4]

assert a.contains(0) == false
assert a.contains(1) == true
assert a.contains(4) == true
assert a.contains(5) == false

assert a.count(0) == 0
assert a.count(1) == 1
assert a.count(4) == 4
assert a.count(5) == 0

assert a.contains(2) == true
assert a.count(2) == 2
assert a.remove(2) == true

assert a.contains(2) == true
assert a.count(2) == 1
assert a.remove(2) == true

assert a.contains(2) == false
assert a.count(2) == 0
assert a.remove(2) == false

assert a == [1, 3, 3, 3, 4, 4, 4, 4]

while a.remove(3)

assert a == [1, 4, 4, 4, 4]


a = [1, 1, 2, 1, 2, 3, 1, 2, 3, 4]

assert a.count(1) == 4

assert a.index(0) == null
assert a.index(1) == 0
assert a.index(2) == 2
assert a.index(3) == 5
assert a.index(4) == 9
assert a.index(5) == null

assert a.index(1) == 0
assert a.index(1, 0) == 0
assert a.index(1, 0) == 0

assert a.index(1, 3, 7) == 3
assert a.index(1, 3, 6) == 3
assert a.index(1, 4, 7) == 6
assert a.index(1, 4, 5) == null

func indices(list, item)
	_indices = []
	i = -1
	while (i = list.index(item, i + 1)) != null
		_indices.add(i)
	return _indices

func rindices(list, item)
	_indices = []
	i = list.size
	while (i = list.rindex(item, i - 1)) != null
		_indices.add(i)
	return _indices

assert indices(a, 0) == []
assert indices(a, 1) == [0, 1, 3, 6]
assert indices(a, 2) == [2, 4, 7]
assert indices(a, 3) == [5, 8]
assert indices(a, 4) == [9]
assert indices(a, 5) == []

assert rindices(a, 0) == []
assert rindices(a, 1) == [6, 3, 1, 0]
assert rindices(a, 2) == [7, 4, 2]
assert rindices(a, 3) == [8, 5]
assert rindices(a, 4) == [9]
assert rindices(a, 5) == []


assert a.index(0, -1000) == null
assert a.index(1, -1000) == 0
assert a.index(2, -1000) == 2

assert a.index(0, 1000) == null
assert a.index(1, 1000) == null
assert a.index(2, 1000) == null

assert a.index(0, -1000, 1000) == null
assert a.index(1, -1000, 1000) == 0
assert a.index(2, -1000, 1000) == 2


assert a.remove(2) == true
assert a == [1, 1, 1, 2, 3, 1, 2, 3, 4]


a = [1, 2, 1, 2, 1]

assert a.index(1, 4) == 4
assert a.index(1, 5) == null

assert a.rindex(1, 0) == 0
assert a.rindex(1, -1) == null

assert a.slice(1, a.size - 1) == [2, 1, 2]
assert a.index(1) == 0
assert a.rindex(1) == 4
assert a.index(1, 0) == 0
assert a.rindex(1, a.size) == 4
assert a.rindex(1, a.size - 1) == 4
assert a.index(1, 1) == 2
assert a.rindex(1, a.size - 2) == 2

i = -1
assert (i = a.index(1, i + 1)) == 0
assert (i = a.index(1, i + 1)) == 2
assert (i = a.index(1, i + 1)) == 4
assert (i = a.index(1, i + 1)) == null

i = a.size
assert (i = a.rindex(1, i - 1)) == 4
assert (i = a.rindex(1, i - 1)) == 2
assert (i = a.rindex(1, i - 1)) == 0
assert (i = a.rindex(1, i - 1)) == null

assert a.index(0, -100, 100) == null
assert a.rindex(0, 100, -100) == null
assert a.index(1, -100, 100) == 0
assert a.rindex(1, 100, -100) == 4
assert a.index(2, -100, 100) == 1
assert a.rindex(2, 100, -100) == 3

# Begin is inclusive, end is inclusive
assert a.index(1, 1, 1) == null
assert a.index(1, 2, 2) == 2
assert a.index(1, 3, 3) == null

assert a.rindex(1, 1, 1) == null
assert a.rindex(1, 2, 2) == 2
assert a.rindex(1, 3, 3) == null

# Begin is inclusive, end is exclusive
assert a.slice(1, 1) == []
assert a.slice(1, 2) == [2]
assert a.slice(1, 3) == [2, 1]
