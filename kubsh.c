#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <sys/wait.h>
#include <readline/readline.h>
#include <readline/history.h>
#include "vfs.h"

#define HISTORY_FILE ".kubsh_history"

static volatile sig_atomic_t signal_received = 0;

// Обработчик сигнала
void sig_handler(int signum) {
    (void)signum;
    signal_received = 1;
    printf("Configuration reloaded\n");
    fflush(stdout);
}

// Убрать кавычки из строки
char* strip_quotes(const char* str) {
    if (!str || strlen(str) < 2) return strdup(str ? str : "");
    
    size_t len = strlen(str);
    if ((str[0] == '\'' && str[len-1] == '\'') ||
        (str[0] == '"' && str[len-1] == '"')) {
        return strndup(str + 1, len - 2);
    }
    return strdup(str);
}

// Команда echo/debug
void cmd_echo(const char* msg) {
    char* stripped = strip_quotes(msg);
    printf("%s\n", stripped);
    free(stripped);
    fflush(stdout);
}

// Вывод переменной окружения
void cmd_env(const char* var_name) {
    char* value = getenv(var_name);
    if (!value) {
        printf("Env variable %s not found\n", var_name);
        fflush(stdout);
        return;
    }
    
    printf("%s\n", value);
    fflush(stdout);
}

// Вывод PATH построчно
void cmd_env_path(void) {
    char* value = getenv("PATH");
    if (!value) {
        printf("Env variable PATH not found\n");
        fflush(stdout);
        return;
    }
    
    char* copy = strdup(value);
    char* token = strtok(copy, ":");
    while (token) {
        printf("%s\n", token);
        token = strtok(NULL, ":");
    }
    free(copy);
    fflush(stdout);
}

// Выполнение внешней команды с проверкой "command not found"
void exec_external(const char* cmd) {
    pid_t pid = fork();
    if (pid == 0) {
        execlp("sh", "sh", "-c", cmd, NULL);
        perror("execlp");
        exit(127);
    } else if (pid > 0) {
        int status;
        waitpid(pid, &status, 0);
        if (WIFEXITED(status) && WEXITSTATUS(status) == 127) {
            printf("%s: command not found\n", cmd);
            fflush(stdout);
        }
    } else {
        perror("fork");
    }
}

// Информация о диске
void disk_info(const char* device) {
    char cmd[256];
    snprintf(cmd, sizeof(cmd), "fdisk -l %s 2>/dev/null || diskutil list %s 2>/dev/null", device, device);
    system(cmd);
}

// Показать информацию о пользователе
void show_user_info(const char* username) {
    struct passwd* pwd = get_user_info(username);
    if (pwd) {
        printf("User information:\n");
        printf("  Username: %s\n", pwd->pw_name);
        printf("  UID: %d\n", pwd->pw_uid);
        printf("  GID: %d\n", pwd->pw_gid);
        printf("  Home: %s\n", pwd->pw_dir);
        printf("  Shell: %s\n", pwd->pw_shell);
    } else {
        printf("User %s not found in system\n", username);
    }
}

int main(void) {
    signal(SIGHUP, sig_handler);
    using_history();
    read_history(HISTORY_FILE);
    
    // Устанавливаем директорию пользователей
    char home_dir[512];
    const char* home = getenv("HOME");
    if (home) {
        snprintf(home_dir, sizeof(home_dir), "%s/users", home);
        set_users_dir(home_dir);
    }
    
    // Инициализируем VFS
    init_users_dir();
    
    printf("\n=== kubsh v1.0 ===\n");
    printf("VFS directory: %s\n", get_users_dir());
    printf("Type '\\help' for available commands\n");
    printf("Type '\\vfs refresh' to populate VFS with system users\n\n");
    
    char* input;
    while (1) {
        if (signal_received) {
            signal_received = 0;
            rebuild_users_vfs();
        }
        
        input = readline("kubsh$ ");
        if (!input) {
            printf("\nExit (Ctrl+D)\n");
            break;
        }
        
        if (*input) add_history(input);
        
        if (strlen(input) == 0) {
            free(input);
            continue;
        }
        
        // Обработка команд
        if (strcmp(input, "\\q") == 0 || strcmp(input, "exit") == 0) {
            printf("Exit\n");
            free(input);
            break;
        }
        else if (strcmp(input, "\\help") == 0 || strcmp(input, "help") == 0) {
            printf("Available commands:\n");
            printf("  \\q, exit          - Exit shell\n");
            printf("  echo <text>       - Print text\n");
            printf("  debug <text>      - Print text (alias for echo)\n");
            printf("  \\l /dev/sda       - Disk information\n");
            printf("  \\e $VAR           - Print environment variable\n");
            printf("  \\e $PATH          - Print PATH entries\n");
            printf("  \\vfs list         - List users in VFS\n");
            printf("  \\vfs refresh      - Rebuild VFS from system users\n");
            printf("  \\vfs info         - VFS information\n");
            printf("  \\vfs create <user>- Create new system user\n");
            printf("  \\vfs delete <user>- Delete system user\n");
            printf("  \\vfs show <user>  - Show user information\n");
            printf("  <command>         - Execute system command\n");
            fflush(stdout);
        }
        else if (strncmp(input, "echo ", 5) == 0) {
            cmd_echo(input + 5);
        }
        else if (strncmp(input, "debug ", 6) == 0) {
            cmd_echo(input + 6);
        }
        else if (strcmp(input, "\\l /dev/sda") == 0) {
            disk_info("/dev/sda");
        }
        else if (strcmp(input, "\\e $HOME") == 0) {
            cmd_env("HOME");
        }
        else if (strcmp(input, "\\e $PATH") == 0) {
            cmd_env_path();
        }
        else if (strncmp(input, "\\e $", 4) == 0) {
            cmd_env(input + 4);
        }
        else if (strcmp(input, "\\vfs list") == 0) {
            list_users_vfs();
        }
        else if (strcmp(input, "\\vfs refresh") == 0) {
            rebuild_users_vfs();
            printf("VFS refreshed at %s\n", get_users_dir());
            fflush(stdout);
        }
        else if (strcmp(input, "\\vfs info") == 0) {
            printf("VFS directory: %s\n", get_users_dir());
            printf("System: %s\n", 
#ifdef __APPLE__
                   "macOS"
#else
                   "Linux"
#endif
                  );
            printf("Commands:\n");
            printf("  \\vfs list         - List all users\n");
            printf("  \\vfs refresh      - Rebuild VFS\n");
            printf("  \\vfs create <user>- Create new user\n");
            printf("  \\vfs delete <user>- Delete user\n");
            printf("  \\vfs show <user>  - Show user info\n");
            printf("\nNote: User creation/deletion may require sudo\n");
            fflush(stdout);
        }
        else if (strncmp(input, "\\vfs create ", 12) == 0) {
            const char* username = input + 12;
            if (strlen(username) > 0) {
                create_user(username);
            } else {
                printf("Error: Please specify username\n");
                printf("Usage: \\vfs create <username>\n");
            }
        }
        else if (strncmp(input, "\\vfs delete ", 12) == 0) {
            const char* username = input + 12;
            if (strlen(username) > 0) {
                printf("Are you sure you want to delete user '%s'? (y/N): ", username);
                fflush(stdout);
                
                char response[10];
                if (fgets(response, sizeof(response), stdin)) {
                    if (response[0] == 'y' || response[0] == 'Y') {
                        delete_user(username);
                    } else {
                        printf("Cancelled\n");
                    }
                }
            } else {
                printf("Error: Please specify username\n");
                printf("Usage: \\vfs delete <username>\n");
            }
        }
        else if (strncmp(input, "\\vfs show ", 10) == 0) {
            const char* username = input + 10;
            if (strlen(username) > 0) {
                show_user_info(username);
            } else {
                printf("Error: Please specify username\n");
                printf("Usage: \\vfs show <username>\n");
            }
        }
        else {
            exec_external(input);
        }
        
        free(input);
    }
    
    write_history(HISTORY_FILE);
    return 0;
}
