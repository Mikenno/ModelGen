
print("begin")
if true
	print("true")
if true print("true")
print("end")

print("begin")
if true print("true")
else print("false")
print("end")

print("begin")
if true print(1) print(2)
else print(3) print(4)
print("end")

print("begin")
if x == y
	print("true")
else
	print("false")
print("end")

print("begin")
if x == y
	print("true")
	if x == y
		print("true true")
	print("true")
else
	print("false")
	if x == y
		print("false true")
	else
		print("false false")
	print("false")
print("end")

for i in range(4)
	print(i)
	if i > 1
		print(i, 1)
		if i > 2
			print(i, 1, 1)
	else
		print(i, 0)
		if i > 0
			print(i, 0, 1)
		else
			print(i, 0, 0)
