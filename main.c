/* Monolithic wrapper and runner for C source files, using in-memory file descriptors and fexecve.
   Copyright (C) 2026  Neeraj R Rugi

   This file is part of CFIRE.

   CFIRE is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   any later version.

   CFIRE is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with CFIRE.  If not, see <https://www.gnu.org/licenses/>.
*/

/* Author: Neeraj R Rugi texonace123@gmail.com */



#define _GNU_SOURCE     //Enable GNU extensions(glibc) in features.h to get memfd_create and fexecve
#include <sys/mman.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

struct timespec start, end;

#define PANIC(...)                                                      \
    do {                                                                \
        fprintf(stderr, "\033[01;31mCFIRE PANIC:\033[0m ");             \
        fprintf(stderr, __VA_ARGS__);                                   \
    } while (0)




int main(int argc, char *argv[]) {

    //Check for input file, at least one argument is required (the C source file)
    if (argc < 2) {
        PANIC("No input file or Makefile argument '-' provided.\n");
        fprintf(stderr, "Usage File: %s file.c [args...]\n", argv[0]);
        fprintf(stderr, "Usage Makefile: %s - [args...]\n", argv[0]);
        exit(1);
    }
    if(strcmp(argv[1], "--panic") == 0) {
        PANIC("PANIC FLAG ACTIVE, Program Exiting.\n");
        exit(1);
    }

    if(strcmp(argv[1], "--time") == 0 || strcmp(argv[1], "--time-gcc") == 0){
        clock_gettime(CLOCK_MONOTONIC, &start);
    }
    //Create in-memory file
    int fd = memfd_create("CFIRE_mem_file", 0);
    if (fd < 0) {
        PANIC("Failed to create in-memory file.\n");
        exit(1);
    }

    char path[64];
    snprintf(path, sizeof(path), "/proc/self/fd/%d", fd);

    /*  Read gcc command from stdin if the first argument is '-',
        otherwise use the first argument as the C source file to compile.
    */
    char sh_cmd[8192] = {0}; // Reassembled shell command for the stdin branch




    if (strcmp(argv[1], "-") == 0 || strcmp(argv[1], "--time-gcc" ) == 0) {
        char cmd_buf[4096];

        if (isatty(STDIN_FILENO))
            fprintf(stderr, "cfire> ");

        int found = 0;
        while (fgets(cmd_buf, sizeof(cmd_buf), stdin)) {
            if (strncmp(cmd_buf, "gcc", 3) == 0) {
                found = 1;
                break;
            }
        }
        // After the while(fgets) scanning loop, before fork()
        
        if (!found) {
            PANIC("No gcc command found in stdin.\n");
            exit(1);
        }

        //Reattach stdin to the terminal so that the compiled program can read from it if needed
        freopen("/dev/tty", "r", stdin);

        /*  Reassemble the command token by token, stripping any existing -o <file>,
            then append -o <memfd path>. The result is passed to sh -c so that
            backtick subshells (e.g. `pkg-config ...`) are expanded by the shell.
        */

        char tmp[4096];
        strncpy(tmp, cmd_buf, sizeof(tmp));

        char *tok = strtok(tmp, " \t\n");
        while (tok) {
            if (strcmp(tok, "-o") == 0) {
                strtok(NULL, " \t\n"); // discard the output filename
                tok = strtok(NULL, " \t\n");
                continue;
            }
            strncat(sh_cmd, tok, sizeof(sh_cmd) - strlen(sh_cmd) - 1);
            strncat(sh_cmd, " ", sizeof(sh_cmd) - strlen(sh_cmd) - 1);
            tok = strtok(NULL, " \t\n");
        }
        // Append -o pointing to the memfd
        strncat(sh_cmd, "-o ", sizeof(sh_cmd) - strlen(sh_cmd) - 1);
        strncat(sh_cmd, path,  sizeof(sh_cmd) - strlen(sh_cmd) - 1);

    }

    //Initiate child process to compile the code into the memfd
    pid_t pid = fork();

    if (pid == 0) {
        if (strcmp(argv[1], "-") == 0 || strcmp(argv[1], "--time-gcc") == 0) {
            // Pass the reassembled command to sh -c so backticks and shell
            // expressions like `pkg-config ...` are properly expanded
            execl("/bin/sh", "sh", "-c", sh_cmd, NULL);
        } else {
            execlp("gcc", "gcc", // Otherwise, compile the provided C source file
                argv[1],
                "-o", path,
                NULL);
        }
        perror("exec");
        exit(1);
        //Child has exited at this point, either successfully or with an error.
    }

    //Wait for compilation
    int status;
    waitpid(pid, &status, 0);
    if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) {
        PANIC("GCC Compilation failed.\n");
        exit(1);
    }

    if(strcmp(argv[1], "--time-gcc") == 0 || strcmp(argv[1], "--time") == 0){
        clock_gettime(CLOCK_MONOTONIC, &end);
        double compilation_time =   (end.tv_sec - start.tv_sec) * 1000.0 + 
                                    (end.tv_nsec - start.tv_nsec) / 1000000.0;
        printf("Build completed in: %.2f ms\n", compilation_time);
        exit(0);
    }

    /*
        To pass arguments to the compiled program, we need to use fexecve which requires the environment variables as well. 
        We can get the environment variables from the current process using the 'environ' variable defined in unistd.h.
    */ 

    char *exec_argv[256];
    exec_argv[0] = "cfire_build";
    int i;
    for (i = 2; i < argc; i++)
        exec_argv[i-1] = argv[i];
    exec_argv[i-1] = NULL;
    fexecve(fd, exec_argv, environ); //Execute the compiled program with the provided arguments and current environment variables

    PANIC("Failed to execute compiled program.\n");
    return 1;
}
