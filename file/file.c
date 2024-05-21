#include <stdlib.h>
#include <stdio.h>
#include <string.h>

int main(void)
{
    FILE *fp = fopen("/disk/test.txt", "rb");
    if (fp) {
        char buf[256];
        fgets(buf, sizeof(buf), fp);
        printf("%s\n", buf);
        fclose(fp);
    }
    while (1) {
        char cmd[256];
        scanf("%255s", cmd);
        if (strcmp(cmd, "exit") == 0 || strcmp(cmd, "quit") == 0) break;
    }
    return 0;
}
