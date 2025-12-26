#include "vfs.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <pwd.h>
#include <dirent.h>
#include <fcntl.h>

// Путь по умолчанию
#define DEFAULT_USERS_DIR "/Users/denisshadrin/users"

static char users_dir[512] = DEFAULT_USERS_DIR;

// Создание каталога VFS
void init_users_dir(void) {
    struct stat st = {0};
    if (stat(users_dir, &st) == -1) {
        if (mkdir(users_dir, 0755) == 0) {
            printf("Created users directory: %s\n", users_dir);
        } else {
            printf("Warning: Cannot create users directory: %s\n", users_dir);
        }
    }
}

// Установка пути VFS
void set_users_dir(const char* dir) {
    if (dir && strlen(dir) > 0) {
        strncpy(users_dir, dir, sizeof(users_dir) - 1);
        users_dir[sizeof(users_dir) - 1] = '\0';
        printf("VFS directory set to: %s\n", users_dir);
    }
}

// Создание файлов id, home, shell для пользователя
void create_user_files(struct passwd* pwd) {
    if (!pwd) return;
    
    char path[512];
    snprintf(path, sizeof(path), "%s/%s", users_dir, pwd->pw_name);
    
    // Создаем директорию пользователя
    struct stat st = {0};
    if (stat(path, &st) == -1) {
        if (mkdir(path, 0755) != 0) {
            printf("Warning: Cannot create directory for user %s\n", pwd->pw_name);
            return;
        }
    }

    char file[512];
    FILE* f;
    
    // id
    snprintf(file, sizeof(file), "%s/id", path);
    f = fopen(file, "w");
    if (f) {
        fprintf(f, "%d\n", pwd->pw_uid);
        fclose(f);
    } else {
        printf("Warning: Cannot create id file for user %s\n", pwd->pw_name);
    }
    
    // home
    snprintf(file, sizeof(file), "%s/home", path);
    f = fopen(file, "w");
    if (f) {
        fprintf(f, "%s\n", pwd->pw_dir);
        fclose(f);
    } else {
        printf("Warning: Cannot create home file for user %s\n", pwd->pw_name);
    }
    
    // shell
    snprintf(file, sizeof(file), "%s/shell", path);
    f = fopen(file, "w");
    if (f) {
        fprintf(f, "%s\n", pwd->pw_shell);
        fclose(f);
    } else {
        printf("Warning: Cannot create shell file for user %s\n", pwd->pw_name);
    }
    
    printf("Created VFS files for user: %s\n", pwd->pw_name);
}

// Создание пользователя в системе
int create_user(const char* username) {
    if (!username || strlen(username) == 0) {
        printf("Error: Invalid username\n");
        return -1;
    }
    
    printf("Creating system user: %s\n", username);
    
    // Проверяем, существует ли уже пользователь
    struct passwd* pwd = getpwnam(username);
    if (pwd != NULL) {
        printf("User %s already exists (UID: %d)\n", username, pwd->pw_uid);
        create_user_files(pwd);
        return 0;
    }
    
    // Команда для создания пользователя
    #ifdef __APPLE__
    // macOS команда
    char cmd[512];
    snprintf(cmd, sizeof(cmd), 
             "sudo dscl . -create /Users/%s 2>/dev/null; "
             "sudo dscl . -create /Users/%s UserShell /bin/bash 2>/dev/null; "
             "sudo dscl . -create /Users/%s RealName '%s' 2>/dev/null; "
             "sudo dscl . -create /Users/%s UniqueID 600 2>/dev/null; "
             "sudo dscl . -create /Users/%s PrimaryGroupID 20 2>/dev/null; "
             "sudo dscl . -create /Users/%s NFSHomeDirectory /Users/%s 2>/dev/null; "
             "sudo mkdir -p /Users/%s 2>/dev/null",
             username, username, username, username, username, username, username, username);
    #else
    // Linux команда
    char cmd[512];
    snprintf(cmd, sizeof(cmd), "sudo useradd -m -s /bin/bash %s 2>/dev/null", username);
    #endif
    
    printf("Executing: %s\n", cmd);
    
    int result = system(cmd);
    if (result == 0) {
        // Получаем информацию о созданном пользователе
        pwd = getpwnam(username);
        if (pwd) {
            create_user_files(pwd);
            printf("User %s created successfully (UID: %d)\n", username, pwd->pw_uid);
            return 0;
        } else {
            printf("User created but cannot get user info\n");
            
            // Создаем временную структуру для VFS
            struct passwd temp_pwd;
            temp_pwd.pw_name = (char*)username;
            temp_pwd.pw_uid = 1000;
            temp_pwd.pw_dir = "/Users/";
            strcat(temp_pwd.pw_dir, username);
            temp_pwd.pw_shell = "/bin/bash";
            create_user_files(&temp_pwd);
            
            return 0;
        }
    } else {
        printf("Failed to create user %s (exit code: %d)\n", username, WEXITSTATUS(result));
        
        // Попробуем создать только VFS структуру без системного пользователя
        printf("Creating only VFS structure for %s\n", username);
        struct passwd temp_pwd;
        temp_pwd.pw_name = (char*)username;
        temp_pwd.pw_uid = 1000;
        temp_pwd.pw_dir = "/Users/";
        strcat(temp_pwd.pw_dir, username);
        temp_pwd.pw_shell = "/bin/bash";
        create_user_files(&temp_pwd);
        return 1;
    }
}

// Удаление пользователя из системы
int delete_user(const char* username) {
    if (!username || strlen(username) == 0) {
        printf("Error: Invalid username\n");
        return -1;
    }
    
    printf("Deleting user: %s\n", username);
    
    // Проверяем, существует ли пользователь
    struct passwd* pwd = getpwnam(username);
    if (pwd == NULL) {
        printf("User %s does not exist in system\n", username);
    } else {
        // Команда для удаления пользователя
        #ifdef __APPLE__
        // macOS команда
        char cmd[512];
        snprintf(cmd, sizeof(cmd), "sudo dscl . -delete /Users/%s 2>/dev/null", username);
        #else
        // Linux команда
        char cmd[512];
        snprintf(cmd, sizeof(cmd), "sudo userdel -r %s 2>/dev/null", username);
        #endif
        
        printf("Executing: %s\n", cmd);
        int result = system(cmd);
        if (result == 0) {
            printf("User %s deleted from system\n", username);
        } else {
            printf("Failed to delete user %s from system (exit code: %d)\n", 
                   username, WEXITSTATUS(result));
        }
    }
    
    // Удаляем VFS директорию пользователя
    char user_dir[512];
    snprintf(user_dir, sizeof(user_dir), "%s/%s", users_dir, username);
    
    if (access(user_dir, F_OK) == 0) {
        // Удаляем файлы пользователя
        char cmd[512];
        snprintf(cmd, sizeof(cmd), "rm -rf \"%s\" 2>/dev/null", user_dir);
        
        if (system(cmd) == 0) {
            printf("Removed VFS directory for user %s\n", username);
        } else {
            printf("Failed to remove VFS directory for user %s\n", username);
        }
    } else {
        printf("VFS directory for user %s does not exist\n", username);
    }
    
    return 0;
}

// Получение информации о пользователе
struct passwd* get_user_info(const char* username) {
    return getpwnam(username);
}

// Перестройка VFS
void rebuild_users_vfs(void) {
    init_users_dir();
    
    struct passwd* pwd;
    setpwent();
    int count = 0;
    
    while ((pwd = getpwent()) != NULL) {
        // Только пользователи с shell
        if (pwd->pw_shell && strstr(pwd->pw_shell, "sh")) {
            create_user_files(pwd);
            count++;
        }
    }
    endpwent();
    
    printf("VFS rebuilt at %s (%d users processed)\n", users_dir, count);
}

// Список пользователей в VFS
void list_users_vfs(void) {
    DIR* dir = opendir(users_dir);
    if (!dir) {
        printf("Cannot open VFS directory: %s\n", users_dir);
        printf("Try running: \\vfs refresh\n");
        return;
    }
    
    printf("Users in VFS (%s):\n", users_dir);
    printf("================================\n");
    
    struct dirent* entry;
    int count = 0;
    
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_type == DT_DIR &&
            strcmp(entry->d_name, ".") != 0 &&
            strcmp(entry->d_name, "..") != 0) {
            
            char user_path[512];
            snprintf(user_path, sizeof(user_path), "%s/%s", users_dir, entry->d_name);
            
            printf("User: %s\n", entry->d_name);
            
            // Показываем информацию из файлов
            char file_path[512];
            FILE* f;
            
            // id
            snprintf(file_path, sizeof(file_path), "%s/id", user_path);
            f = fopen(file_path, "r");
            if (f) {
                char id[256];
                if (fgets(id, sizeof(id), f)) {
                    printf("  UID: %s", id);
                }
                fclose(f);
            }
            
            // home
            snprintf(file_path, sizeof(file_path), "%s/home", user_path);
            f = fopen(file_path, "r");
            if (f) {
                char home[256];
                if (fgets(home, sizeof(home), f)) {
                    printf("  Home: %s", home);
                }
                fclose(f);
            }
            
            // shell
            snprintf(file_path, sizeof(file_path), "%s/shell", user_path);
            f = fopen(file_path, "r");
            if (f) {
                char shell[256];
                if (fgets(shell, sizeof(shell), f)) {
                    printf("  Shell: %s", shell);
                }
                fclose(f);
            }
            
            printf("\n");
            count++;
        }
    }
    
    closedir(dir);
    
    if (count == 0) {
        printf("No users found in VFS\n");
        printf("Run '\\vfs refresh' to populate VFS\n");
    } else {
        printf("Total: %d users\n", count);
    }
}

// Получение пути к VFS
const char* get_users_dir(void) {
    return users_dir;
}
