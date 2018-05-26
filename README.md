
# ModelGen

ModelGen is a language for generating 3D models.
In contrast to existing modeling programs, ModelGen allows users to create models programmatically, rather than design them through an interactive GUI.

Note that ModelGen is not a sculpting tool, but instead a tool for parametric modeling.

## Tools

| Name | Description |
| ------------- | ------------- |
| [VSCode Extension][vscode] | Syntax highlighting, code completion and more for VSCode |
| [Android Model Viewer][android] | Execute ModelGen code and preview models on Android |

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

## Test Suite

```bash
python3 build.py debug test
./bin/modelgen-test
```

  [vscode]: https://github.com/MrVallentin/ModelGen-VSCode
  [android]: https://github.com/MrVallentin/ModelGen-Android-Viewer
