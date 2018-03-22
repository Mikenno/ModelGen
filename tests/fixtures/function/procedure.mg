
proc test1()
print("Outside")

proc test2() print("test2") print("test2")
print("Outside")

proc test3()
	print("test3")
	print("test3")
print("Outside")

proc test4(x)
	print("test4", x)

proc test5(x, y, z)
	print("test5", x, y, z)

proc test6(x, y = 1, z = 1 + 2)
	print("test6", x, y, z)
