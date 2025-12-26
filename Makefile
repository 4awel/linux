# -------------------------
# Компилятор и флаги
# -------------------------
CC = gcc
CFLAGS = -Wall -Wextra -O2 -std=c11 -D_GNU_SOURCE

# Определение операционной системы
UNAME_S := $(shell uname -s)

# Настройки для разных ОС
ifeq ($(UNAME_S),Linux)
    # Linux
    LDFLAGS = -lreadline
    USERS_DIR = $(HOME)/users
else ifeq ($(UNAME_S),Darwin)
    # macOS
    LDFLAGS = -lreadline
    USERS_DIR = $(HOME)/users
else
    # Другие системы
    LDFLAGS = -lreadline
    USERS_DIR = ./users
endif

TARGET = kubsh

.PHONY: all clean run install-deps install-local uninstall test help

# -------------------------
# Сборка
# -------------------------
all: $(TARGET)

$(TARGET): vfs.o kubsh.o
	$(CC) $(CFLAGS) vfs.o kubsh.o -o $(TARGET) $(LDFLAGS)

vfs.o: vfs.c vfs.h
	$(CC) $(CFLAGS) -c vfs.c -o vfs.o

kubsh.o: kubsh.c vfs.h
	$(CC) $(CFLAGS) -c kubsh.c -o kubsh.o

# -------------------------
# Создание директории пользователей
# -------------------------
create-users-dir:
	mkdir -p $(USERS_DIR) && chmod 0755 $(USERS_DIR)

# -------------------------
# Запуск
# -------------------------
run: create-users-dir $(TARGET)
	./$(TARGET)

# -------------------------
# Установка зависимостей для macOS
# -------------------------
install-deps-macos:
	brew install readline

# -------------------------
# Установка зависимостей для Linux
# -------------------------
install-deps-linux:
	sudo apt-get update
	sudo apt-get install -y build-essential libreadline-dev

# -------------------------
# Установка зависимостей (автовыбор)
# -------------------------
install-deps:
ifeq ($(UNAME_S),Linux)
	$(MAKE) install-deps-linux
else ifeq ($(UNAME_S),Darwin)
	$(MAKE) install-deps-macos
else
	@echo "Неизвестная ОС: $(UNAME_S)"
endif

# -------------------------
# Установка локально для тестов (без root)
# -------------------------
install-local: $(TARGET)
	@mkdir -p ~/bin
	ln -sf $(PWD)/$(TARGET) ~/bin/$(TARGET)
	@echo "kubsh установлен в ~/bin (добавьте ~/bin в PATH, если ещё не добавлено)"

uninstall:
	rm -f ~/bin/$(TARGET)

# -------------------------
# Очистка
# -------------------------
clean:
	rm -rf $(TARGET) *.o

# -------------------------
# Тест
# -------------------------
test: $(TARGET)
	@echo "=== Тестирование kubsh ==="
	@echo "1. Запустите: make run"
	@echo "2. Внутри kubsh выполните:"
	@echo "   \\vfs refresh      # Заполнить VFS"
	@echo "   \\vfs list         # Список пользователей"
	@echo "   \\vfs create testuser  # Создать тестового пользователя"
	@echo "   \\vfs show testuser    # Информация о пользователе"
	@echo "   \\vfs delete testuser  # Удалить пользователя"
	@echo ""
	@echo "Note: Для создания/удаления пользователей может потребоваться sudo"

help:
	@echo "Доступные команды:"
	@echo "  make all              - Собрать проект"
	@echo "  make run              - Собрать и запустить шелл"
	@echo "  make install-deps     - Установить зависимости"
	@echo "  make clean            - Очистить сборочные файлы"
	@echo "  make test             - Инструкции для теста"
	@echo "  make install-local    - Установить локально"
