#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define print_err(...) fprintf(stderr, __VA_ARGS__)

/* entry point. */
int main(int argc, const char** argv) {
    if (argc < 3) {
        print_err(
            "usage: %s <source.asm> <output.obj> [symbol]\n"
            "e.g. symbol: test -> test_b, test_e. \n", argv[0]);

        return 1;
    }

    char cmdbuf[256];
    char pathBuf[256];

    memset(cmdbuf, 0, sizeof(cmdbuf));
    snprintf(cmdbuf, sizeof(cmdbuf) - 1, "nasm -f bin %s -o %s.tmp", argv[1], argv[1]);

    if (system(cmdbuf)) {
        print_err(
            "%s: nasm: failed to assemble source code: %s.\n\n", 
            argv[0], argv[1]);

        return 1;
    }
    
    memset(cmdbuf, 0, sizeof(cmdbuf));
    memset(pathBuf, 0, sizeof(pathBuf));
    strncpy(pathBuf, argv[0], sizeof(pathBuf) - 1);

    if (const char* slash = strrchr(pathBuf, '/')) {
        *((char*) slash) = '\0';
    }

    if (argc > 3) {
        snprintf(cmdbuf, sizeof(cmdbuf) - 1, "%s/bin2elfo %s.tmp %s %s", pathBuf, argv[1], argv[2], argv[3]);
    } else {
        snprintf(cmdbuf, sizeof(cmdbuf) - 1, "%s/bin2elfo %s.tmp %s", pathBuf, argv[1], argv[2]);
    }
    
    if (system(cmdbuf)) {
        print_err(
            "%s: bin2elfo: failed to convert to ELF64 object: %s.\n\n", 
            argv[0], argv[1]);

        return 1;
    }

    memset(cmdbuf, 0, sizeof(cmdbuf));
    snprintf(cmdbuf, sizeof(cmdbuf) - 1, "%s.tmp", argv[1]);

    unlink(cmdbuf);
    return 0;
}
