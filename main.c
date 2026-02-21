#define _GNU_SOURCE
#include <sys/mman.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>


#define PANIC(...)                                \
    do {                                          \
        fprintf(stderr, "\033[31mCFIRE PANIC:\033[0m "); \
        fprintf(stderr, __VA_ARGS__);                      \
    } while (0)

int main(int argc, char *argv[]) {
    //Check for input file, at least one argument is required (the C source file)
    if (argc < 2) {
        PANIC("No input file provided.\n");
        fprintf(stderr, "Usage: %s file.c [args...]\n", argv[0]);
        exit(1);
    }
    //Create in-memory file
    int fd = memfd_create("mem_file", 0);
    if (fd < 0) {
        PANIC("Failed to create in-memory file.\n");
        exit(1);
    }
    //Intiate child process to compile the code into the memfd
    pid_t pid = fork();
    if (pid == 0) {
        // Child: compile into the memfd
        char path[64];
        snprintf(path, sizeof(path), "/proc/self/fd/%d", fd);

        execlp("gcc", "gcc",
               argv[1],
               "-o", path,
               NULL);

        perror("execlp");
        exit(1);
    }

    //Wait for compilation
    int status;
    waitpid(pid, &status, 0);
    if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) {
        PANIC("GCC Compilation failed.\n");
        exit(1);
    }

    

    /*
        To pass arguments to the compiled program, we need to use fexecve which requires the environment variables as well. 
        We can get the environment variables from the current process using the 'environ' variable defined in unistd.h.
    */ 
    extern char **environ;
    fexecve(fd, &argv[1], environ);//Execute the compiled program with the provided arguments and current environment variables

    PANIC("Failed to execute compiled program.\n");
    return 1;
}