
a = {"b": {}}
a.b["c"] = {}

func a.b.c.test(x = a)
	print(a.b.i)
	a["b"].i += a.b["i"]
	return x

a.b["i"] = 1

a.b.c.test()
a.b.c.test(a).b["c"].test(a.b)["c"].test(a.b.c)["test"]()
