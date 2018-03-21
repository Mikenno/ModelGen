
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
