import sys

if (len(sys.argv) <= 1):
    print("Please pass the path")
    exit()

f = open(sys.argv[1],"r")
lines = f.readlines()

verticies = []

print

for line in lines: # automaticly ignores #, o, s, vn ...
    if (line.startswith("v")):
        data = line.strip("\n").split(" ")[1:]
        verticies.append(data)

    if (line.startswith("f")):
        indicies = line.strip("\n").split(" ")[1:]
        print("add( make_shared<Triangle>(", end="")
        print("vec3({},{},{}),".format(*verticies[int(indicies[0]) - 1]), end="")
        print("vec3({},{},{}),".format(*verticies[int(indicies[1]) - 1]), end="")
        print("vec3({},{},{})".format(*verticies[int(indicies[2]) - 1]), end="")
        print(", m));\n", end="")