# CFIRE - Compile and Fire

A blazingly fast C program runner that compiles and executes C code entirely in memory, without writing to disk.

## Overview

CFIRE is a "Compile and Go" wrapper around GCC that leverages Linux's in-memory file system to eliminate I/O overhead. It reads a C source file, compiles it to memory using `memfd_create`, and executes the compiled binary directly—all without touching your disk.

This approach is particularly useful for:
- **Rapid prototyping** - Compile and test C code without generating intermediate binary files
- **Scripting-like execution** - Run C programs similar to how you'd run scripts
- **Performance testing** - Benchmark code without disk I/O interference
- **Sandboxing** - Execute untrusted code without persistent file artifacts
- **Educational tools** - Quickly execute student submissions without file clutter

## Features

- **Zero-disk overhead**: Compiled binaries exist only in memory
- **GCC-based**: Leverages the full power of GCC's optimization and compilation
- **Direct execution**: No intermediate steps between compilation and execution
- **Clean environment**: No binary artifacts left on the file system
- **Simple interface**: Minimal command-line complexity
- **Full compatibility**: Pass arguments to your C program just like normal execution

## How It Works

CFIRE operates through a three-stage process:

### 1. Memory File Creation
```c
int fd = memfd_create("mem_file", 0);
```
Creates an anonymous in-memory file descriptor that behaves like a regular file but exists entirely in RAM.

### 2. Compilation in Child Process
```c
pid_t pid = fork();
// Child process redirects GCC output to the memfd
execlp("gcc", "gcc", argv[1], "-o", path_to_memfd, NULL);
```
A child process compiles the source code directly into the memory file using GCC. The compiled binary never touches the disk.

### 3. Execution via fexecve
```c
fexecve(fd, &argv[1], environ);
```
The program executes the in-memory binary using `fexecve`, which loads and runs the compiled code from the file descriptor.

## Architecture

```
Input C File
     ↓
[CFIRE Main Process]
     ↓
  fork()
     ├─> [Child: GCC]  →  Compile to memfd
     │
  waitpid()
     ↓
  [fexecve] → Execute from memfd → Output
```

## Prerequisites

- **Linux kernel** with support for `memfd_create` (Linux 3.17+)
- **GCC** compiler
- **glibc** with GNU extensions (for `_GNU_SOURCE`)

## Building

### Basic Compilation
```bash
gcc main.c -o cfire
```

### With Optimization
```bash
gcc -O2 main.c -o cfire
```

### With Debug Symbols
```bash
gcc -g main.c -o cfire
```

## Usage

### Basic Syntax
```bash
./cfire <source_file.c> [arguments...]
```

### Parameters
- `<source_file.c>` - Path to the C source file to compile and execute
- `[arguments...]` - Optional arguments passed to the compiled program

## Examples

### Simple Hello World
Create `hello.c`:
```c
#include <stdio.h>

int main() {
    printf("Hello from CFIRE!\n");
    return 0;
}
```

Execute:
```bash
./cfire hello.c
```

Output:
```
Hello from CFIRE!
```

### Program with Arguments
Create `greet.c`:
```c
#include <stdio.h>

int main(int argc, char *argv[]) {
    printf("Hello, %s!\n", argc > 1 ? argv[1] : "World");
    return 0;
}
```

Execute:
```bash
./cfire greet.c Alice
```

Output:
```
Hello, Alice!
```

### File I/O Example
Create `fileops.c`:
```c
#include <stdio.h>

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <filename>\n", argv[0]);
        return 1;
    }
    
    FILE *f = fopen(argv[1], "r");
    if (!f) {
        perror("fopen");
        return 1;
    }
    
    char buffer[256];
    while (fgets(buffer, sizeof(buffer), f)) {
        printf("%s", buffer);
    }
    
    fclose(f);
    return 0;
}
```

Execute:
```bash
./cfire fileops.c /etc/hostname
```

The compiled binary exists only in memory, but can still read from and interact with the file system normally.

## Technical Details

### System Calls Used

| System Call | Purpose |
|------------|---------|
| `memfd_create()` | Create anonymous in-memory file |
| `fork()` | Create child process for compilation |
| `execlp()` | Execute GCC in child process |
| `waitpid()` | Wait for compilation to complete |
| `fexecve()` | Execute compiled binary from file descriptor |

### Memory File Descriptor Path

The program passes GCC the path `/proc/self/fd/<fd>`, which:
- References the memory file descriptor from GCC's perspective
- Works because GCC receives the path before `fexecve` changes the process image
- Allows GCC to write the compiled binary directly to memory

### Error Handling

CFIRE implements error checking for:
- Missing input file argument
- Memory file creation failure
- GCC compilation failure (non-zero exit code)
- Binary execution failure

All errors are prefixed with a colored `CFIRE PANIC:` message for visibility.

## Performance Considerations

### Advantages
- **No disk I/O** for compilation artifacts
- **Faster iteration** during development
- **Reduced SSD wear** from frequent read/write cycles
- **No cleanup** needed after execution

## Comparison with Traditional Execution

### Traditional GCC Approach
```bash
gcc source.c -o binary
./binary arg1 arg2
rm binary  # cleanup
```
- Creates persistent binary file on disk
- Three separate commands required
- Binary remains on disk

### CFIRE Approach
```bash
./cfire source.c arg1 arg2
```
- In-memory compilation and execution
- Single command
- No file cleanup needed

## Limitations and Considerations

1. **No Binary Persistence**: The compiled binary only exists during execution. If you need to run the same code multiple times, it will be recompiled each time. CFIRE is effective for making small to medium codebases especially when iterating quikly, allowing for a pseudo `JIT/HOT Compiler` in spirit.

2. **GCC Compilation Overhead**: All compilation time is incurred on each run. For large projects, consider traditional compilation.

3. **Memory Constraints**: Very large binaries may consume significant RAM. Monitor memory usage for complex programs.

4. **Linux-Only**: This tool requires Linux with `memfd_create` support and will not work on macOS or Windows.


## Advanced Usage

### Timing Execution
```bash
time ./cfire program.c arg1 arg2
```

### Capturing Output
```bash
./cfire program.c > output.txt 2>&1
```

### Piping Data
```bash
echo "input data" | ./cfire program.c
```

### Debugging
To debug the CFIRE mechanism itself, you can:
```bash
gcc -g main.c -o cfire
gdb ./cfire
```

To add debug output to the main program, compile with preprocessing symbols:
```bash
gcc -DDEBUG -g main.c -o cfire
```

## Implementation Notes

### Key Code Sections

The main implementation consists of three critical sections:

1. **Memory File Setup** - Creates the anonymous file descriptor
2. **Child Process Compilation** - Forks and runs GCC targeting the memfd
3. **Program Execution** - Uses `fexecve` to load and run the compiled binary

### Why fexecve?

`fexecve` is essential because:
- It executes a program from a file descriptor rather than a file path
- The binary exists in memory without a traditional file path
- It maintains the proper environment for program execution

## Building from Source

```bash
# Clone or navigate to the project directory
cd /path/to/cfire

# Compile CFIRE
gcc -O2 main.c -o cfire

# Verify installation
./cfire --help  # This will fail without an input file, as expected
```

## License

This project is provided under the GNU GPLv3 License

## Contributing

This is a student project, meant to enhance their workflow, do not expect much work on this.

## Future Enhancements

Potential improvements for CFIRE:
- **Compiler selection**: Support Clang, ICC, etc.
- **C++ support**: Extend to C++ compilation
- **Binary caching**: Cache compiled binaries in memory for rapid re-execution
- **Compilation flags**: Allow customization of GCC flags
- **IDEs support**: Integration with development environments
- **Cross-compilation**: Target different architectures
- **Static linking**: Option for static binary compilation

## References

- [memfd_create(2) - Linux man pages online](https://man7.org/linux/man-pages/man2/memfd_create.2.html)
- [fexecve(3) - Linux man pages online](https://man7.org/linux/man-pages/man3/fexecve.3.html)
- [GCC Manual](https://gcc.gnu.org/onlinedocs/gcc/)
- [Process Management - The Linux Programming Interface](https://man7.org/linux/man-pages/)

---

**CFIRE** - Compile code, execute instantly, leave no trace.
