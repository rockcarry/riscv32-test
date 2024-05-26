#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include "libffvm.h"
#include "fftask.h"

static void cmd_to_argc_argv(int *argc, char *argv[256], char *cmd)
{
    int token, i = 0;
    while ((token = *cmd) && i < 256) {
        if (token == ' ' || token == '\r' || token == '\n') {
        } else if (token == '"') {
            argv[i++] = ++cmd;
            while (*cmd && *cmd != '"') cmd++;
        } else if (token == '\'') {
            argv[i++] = ++cmd;
            while (*cmd && *cmd != '\'') cmd++;
        } else {
            argv[i++] = cmd;
            while (*cmd && *cmd != ' ' && *cmd != '\r' && *cmd != '\n') cmd++;
        }
        if (*cmd) *cmd++ = '\0';
    }
    *argc = i;
}

static int is_dir(char *path)
{
    DIR *dir = opendir(path);
    if (dir) closedir(dir);
    return !!dir;
}

static int is_file(char *path)
{
    FILE *fp = fopen(path, "rb");
    if (fp) fclose(fp);
    return !!fp;
}

static void shell_rmdir(char *path)
{
    DIR *dir = opendir(path);
    if (dir) {
        chdir(path);
        struct dirent *ent = NULL;
        while ((ent = readdir(dir)) != NULL) {
            if (ent->d_type != DT_DIR) unlink(ent->d_name);
            else shell_rmdir(ent->d_name);
        }
        closedir(dir);
        chdir("..");
    }
    unlink(path);
}

static void copy_file(char *src, char *dst)
{
    FILE *fpsrc = fopen(src, "rb");
    FILE *fpdst = fopen(dst, "wb");
    if (fpsrc && fpdst) {
        while (1) { char c = fgetc(fpsrc); if (feof(fpsrc)) break; fputc(c, fpdst); }
    } else {
        printf("cp: failed to copy file %s -> %s\n", src, dst);
    }
    if (fpsrc) fclose(fpsrc);
    if (fpdst) fclose(fpdst);
}

static void copy_dir(char *src, char *dst)
{
    DIR *dir = opendir(src);
    char newsrc[257], newdst[257];
    if (dir) {
        mkdir(dst, 0);
        struct dirent *ent = NULL;
        while ((ent = readdir(dir)) != NULL) {
            snprintf(newsrc, sizeof(newsrc), "%s/%s", src, ent->d_name);
            snprintf(newdst, sizeof(newdst), "%s/%s", dst, ent->d_name);
            if (ent->d_type != DT_DIR) copy_file(newsrc, newdst);
            else copy_dir(newsrc, newdst);
        }
        closedir(dir);
    }
}

int main(void)
{
    task_kernel_init();
    while (1) {
        int   argc, i;
        char *argv[256], cmd[256], buf[256];
        getcwd(cmd, sizeof(cmd));
        printf("ffsh: %s $ ", cmd);
        fgets(cmd, sizeof(cmd), stdin);
        cmd_to_argc_argv(&argc, argv, cmd);
//      for (i = 0; i < argc; i++) printf("%s\n", argv[i]);
        if (strcmp(argv[0], "pwd") == 0) {
            getcwd(cmd, sizeof(cmd));
            printf("%s\n", cmd);
        } else if (strcmp(argv[0], "ls") == 0) {
            DIR *dir = opendir(argc < 2 ? "." : argv[1]);
            struct dirent *ent = NULL;
            while ((ent = readdir(dir)) != NULL) {
                char dirorsize[33];
                if (ent->d_type == DT_DIR) snprintf(dirorsize, sizeof(dirorsize), "<dir>");
                else snprintf(dirorsize, sizeof(dirorsize), "%12lld", ent->d_fsize);
                printf("%04d-%02d-%02d %02d:%02d:%02d   %20s   %s\n",
                    (ent->d_fdate >> 9) + 1980, (ent->d_fdate >> 5) & 0xF, ent->d_fdate & 0x1F,
                    (ent->d_ftime >> 11) & 0x1F, (ent->d_fdate >> 5) & 0x3F, (ent->d_fdate & 0x1F) * 2,
                    dirorsize, ent->d_name);
            }
            closedir(dir);
        } else if (strcmp(argv[0], "mkdir") == 0) {
            if (argc < 2) printf("mkdir: missing operand !\n");
            else for (i = 1; i < argc; i++ ) mkdir(argv[i], 0);
        } else if (strcmp(argv[0], "cd") == 0) {
            if (argc < 2) printf("chdir: missing operand !\n");
            chdir(argv[1]);
        } else if (strcmp(argv[0], "cat") == 0) {
            if (argc < 2) { printf("cat: missing operand !\n"); continue; }
            FILE *fp = fopen(argv[1], "rb");
            if (!fp) printf("cat: %s: no such file or directory !\n", argv[1]);
            else { while (!feof(fp)) fputc(fgetc(fp), stdout); fputc('\n', stdout); }
        } else if (strcmp(argv[0], "echo") == 0) {
            char *mode = "wb";
            if (argc < 2) continue;
            if (argc < 4) { printf("%s\n", argv[1]); continue; }
            if (strcmp(argv[2], ">") == 0) mode = "wb";
            else if (strcmp(argv[2], ">>") == 0) mode = "rb+";
            else { printf("%s\n", argv[1]); continue; }
            FILE *fp = fopen(argv[3], mode);
            if (!fp) { printf("echo: failed to open file: %s !\n", argv[3]); continue; }
            else fwrite(argv[1], 1, strlen(argv[1]), fp);
            fclose(fp);
        } else if (strcmp(argv[0], "mv") == 0) {
            if (argc < 3) { printf("mv: missing operand !\n"); continue; }
            if (is_dir(argv[2])) snprintf(buf, sizeof(buf), "%s/%s", argv[2], argv[1]);
            else snprintf(buf, sizeof(buf), "%s", argv[2]);
            rename(argv[1], buf);
        } else if (strcmp(argv[0], "cp") == 0) {
            if (argc < 3) { printf("cp: missing operand !\n");; continue; }
            int flag = strcmp(argv[1], "-r") == 0;
            if (!is_dir(argv[1 + flag]) && !is_file(argv[1 + flag])) { printf("cp: '%s' no such file or directory !\n", argv[1 + flag]); continue; }
            if (!flag) {
                if (is_dir(argv[1])) { printf("cp: -r not specified; omitting directory '%s'\n", argv[1]); continue; }
                if (is_dir(argv[2])) snprintf(buf, sizeof(buf), "%s/%s", argv[2], argv[1]);
                else snprintf(buf, sizeof(buf), "%s", argv[2]);
                copy_file(argv[1], buf);
            } else {
                if (argc < 4) { printf("cp: missing operand !\n");; continue; }
                if (is_dir(argv[2]) && is_file(argv[3])) { printf("cp: cannot overwrite non-directory '%s' with directory '%s'\n", argv[3], argv[2]); continue; }
                if (is_dir(argv[3])) snprintf(buf, sizeof(buf), "%s/%s", argv[3], argv[2]);
                else snprintf(buf, sizeof(buf), "%s", argv[3]);
                if (is_file(argv[2])) copy_file(argv[2], buf);
                else copy_dir(argv[2], buf);
            }
        } else if (strcmp(argv[0], "rm") == 0) {
            if (argc < 2) printf("rm: missing operand !\n");
            int flag = strcmp(argv[1], "-r") == 0;
            if (!flag) {
                for (i = 1; i < argc; i++) unlink(argv[i]);
            } else {
                for (i = 2; i < argc; i++) shell_rmdir(argv[i]);
            }
        } else if (strcmp(argv[0], "exit") == 0 || strcmp(argv[0], "quit") == 0) {
            break;
        } else {
            printf("ffsh: %s: command not found\n", argv[0]);
        }
    }
    task_kernel_exit();
    return 0;
}
