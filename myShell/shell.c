#include "shell.h"
#include "log.h"
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>

extern log_t Log;

// 兼容没有strdup的环境
static char* my_strdup(const char* s) {
    size_t len = strlen(s) + 1;
    char* d = (char*)malloc(len);
    if (d) memcpy(d, s, len);
    return d;
}

// /path/to/cwd$ 
void prefix() {
    char cwd[1024];
    if (getcwd(cwd, sizeof(cwd))) {
        printf("%s$ ", cwd);
        fflush(stdout);
    }
}

// 栈底到栈顶递归输出历史
static void print_log_from_bottom(node* cur) {
    if (!cur) return;
    print_log_from_bottom(cur->next);
    printf("%s\n", cur->cmd);
}

// 查找最近的历史命令（非!开头），前缀匹配
static char* find_cmd_by_prefix(node* cur, const char* prefix) {
    size_t len = strlen(prefix);
    while (cur) {
        if (cur->cmd[0] != '!' && strncmp(cur->cmd, prefix, len) == 0) {
            return cur->cmd;
        }
        cur = cur->next;
    }
    return NULL;
}

int execute(char* buffer) {
    // 判断是否以!开头
    int is_history_cmd = (buffer[0] == '!');
    // !# 显示历史
    if (strcmp(buffer, "!#") == 0) {
        print_log_from_bottom(Log.head);
        return 1;
    }
    // !prefix 查找并执行历史命令
    if (is_history_cmd && strlen(buffer) > 1) {
        char* prefix = buffer + 1;
        char* found = find_cmd_by_prefix(Log.head, prefix);
        if (found) {
            // 递归执行历史命令（该命令会被push入历史）
            char* copy = my_strdup(found);
            int ret = execute(copy);
            free(copy);
            return ret;
        } else {
            printf("No Match\n");
            return 1;
        }
    }
    // 其它命令（非!开头）全部入历史
    if (!is_history_cmd) {
        log_push(&Log, buffer);
    }
    // 分割参数
    char* args[100];
    int arg_count = 0;
    char buf_copy[1024];
    strncpy(buf_copy, buffer, sizeof(buf_copy) - 1);
    buf_copy[sizeof(buf_copy) - 1] = '\0';
    char* token = strtok(buf_copy, " \t");
    while (token && arg_count < 99) {
        args[arg_count++] = token;
        token = strtok(NULL, " \t");
    }
    args[arg_count] = NULL;
    if (arg_count == 0) return 1;

    // cd
    if (strcmp(args[0], "cd") == 0) {
        const char* dir = NULL;
        if (arg_count == 1) {
            dir = getenv("HOME");
        } else if (arg_count == 2) {
            dir = args[1];
        } else {
            printf("cd: too many arguments\n");
            return 1;
        }
        if (dir && chdir(dir) != 0) {
            printf("%s: No such file or directory\n", args[1] ? args[1] : dir);
        }
        return 1;
    }
    // exit
    if (strcmp(args[0], "exit") == 0) {
        return 0;
    }
    // ls
    if (strcmp(args[0], "ls") == 0) {
        system("ls");
        return 1;
    }
    // 其它外部命令
    pid_t pid = fork();
    if (pid == 0) {
        execvp(args[0], args);
        printf("%s: no such command\n", args[0]);
        exit(1);
    } else if (pid > 0) {
        int status;
        waitpid(pid, &status, 0);
        return 1;
    } else {
        return 1;
    }
}