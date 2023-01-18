* YAPL (Yet another programming language)

** Usage

Run the executable from a shell with a path to some source code as the only argument.

** Building

Dependencies

- CMake >= 3.14

- Any C Compiler

FIRST, generate a build tree.
```bash
cmake -B bld
```

Finally, build an executable from the build tree.
```bash
cmake --build bld
```
