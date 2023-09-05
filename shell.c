#include "user.h"

void main(void) {
    while (1) {
    prompt:  // goto label (yes, goto is bad, but this is a kernel, not a web app)
        printf("> ");
        char cmdline[128];
        for (int i = 0;; i++) {
            char ch = getchar();
            putchar(ch);
            if (i == sizeof(cmdline) - 1) {
                printf("command line too long\n");
                goto prompt;
            } else if (ch == '\r') {
                printf("\n");
                cmdline[i] = '\0';
                break;
            } else {
                cmdline[i] = ch;
            }
        }

        if (strcmp(cmdline, "hello") == 0) {
            printf("Hello world from shell!\n");
        } else if (strcmp(cmdline, "exit") == 0) {
            exit();
        } else if (strncmp(cmdline, "echo ", 4) == 0) {
            printf("%s\n", cmdline + 5);
        } else {
            printf("unknown command: %s\n", cmdline);
        }
    }
}