#ifndef VFS_H
#define VFS_H

#include <pwd.h>

#ifdef __cplusplus
extern "C" {
#endif

// Инициализация директории пользователей
void init_users_dir(void);

// Создание файлов для пользователя
void create_user_files(struct passwd* pwd);

// Перестройка VFS
void rebuild_users_vfs(void);

// Список пользователей в VFS
void list_users_vfs(void);

// Получение пути к VFS
const char* get_users_dir(void);

// Установка пути VFS
void set_users_dir(const char* dir);

// Создание пользователя
int create_user(const char* username);

// Удаление пользователя
int delete_user(const char* username);

// Получение информации о пользователе
struct passwd* get_user_info(const char* username);

#ifdef __cplusplus
}
#endif

#endif // VFS_H
