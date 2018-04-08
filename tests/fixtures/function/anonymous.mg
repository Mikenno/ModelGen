
(proc() print("Anonymous"))

(proc() print("Anonymous"))()
(((proc() print("Anonymous"))))()

(proc(x, y) print("Anonymous", x, y))(1, 2)

test1 = proc() print("Anonymous")
test2 = (proc() print("Anonymous"))
test3 = proc()
	print("Anonymous")
	print("Anonymous")

test4 = (proc()
	print("Anonymous")
	print("Anonymous")
)

(proc(), proc(), proc())
((proc() print(1)), (proc() print(2)), (proc() print(3)))

((proc test1() print(1)), (proc test2() print(2)), (proc test3() print(3)))

x = (
	(proc() print("Anonymous 1")),
	(proc() print("Anonymous 2")),
	(proc() print("Anonymous 3"))
)

y = (
	(proc() print("Anonymous 1")),
	(proc() print("Anonymous 2")),
	(proc() print("Anonymous 3"))
)

z = (
	(proc() print("Anonymous 1"))(),
	(proc() print("Anonymous 2"))(),
	(proc() print("Anonymous 3"))()
)
