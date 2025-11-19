# MIPS Pipeline Simulator

## Requirements

- C++17 compatible compiler (g++, clang++)
- Standard C++ libraries

## Building

Compile the program using g++ with C++17 standard:

```bash
cd main_files
g++ -std=c++17 -Wall -Wextra main.cpp mips_pipeline.cpp mips_output.cpp -o mips_sim
```

Or use the shorter version:

```bash
cd main_files
g++ -std=c++17 *.cpp -o mips_sim
```

## Running

### Run with an input file:

```bash
./mips_sim <input_file.asm>
```

Example:
```bash
./mips_sim test.asm
```

### Run with stdin:

```bash
./mips_sim
```

Then type your MIPS instructions (one per line). Press Ctrl+D (Unix) or Ctrl+Z (Windows) to finish input.

Example:
```bash
echo "ADDI $8, $0, 10" | ./mips_sim
```
