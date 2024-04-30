#include <stdlib.h>
#include <stdio.h>

int main(void)
{
    FILE *fp = fopen("/disk0/test.txt", "rb");
    if (fp) {
        char buf[256];
        fgets(buf, sizeof(buf), fp);
        printf("%s\n", buf);
        fclose(fp);
    }
    return 0;
}
