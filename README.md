
# ModelGen

ModelGen is a language for generating 3D models.
In contrast to existing modeling programs, ModelGen allows users to textually describe models, rather than design them through an interactive GUI.

Note that ModelGen is not a sculpting tool, but instead a tool for parametric modeling.

## Building

```bash
python3 build.py debug
./bin/modelgen-debug --help
```

## Running

```bash
echo 'print("Hello World")' | modelgen --stdin

# or

echo 'print("Hello World")' > test.mg
./bin/modelgen-debug test.mg
```
