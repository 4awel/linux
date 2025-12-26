#!/bin/bash
echo "=== Тестирование kubsh ==="

# Компиляция
echo "1. Компиляция..."
make clean
make all
if [ $? -ne 0 ]; then
    echo "Ошибка компиляции!"
    exit 1
fi
echo "✓ Компиляция успешна"

# Проверка бинарника
echo "2. Проверка бинарника..."
if [ ! -f "./kubsh" ]; then
    echo "Бинарник kubsh не найден!"
    exit 1
fi
file ./kubsh
echo "✓ Бинарник создан"

# Тест помощи
echo "3. Тест VFS информации..."
echo "\vfs info" | ./kubsh | head -5

# Тест списка пользователей
echo "4. Тест списка пользователей..."
echo "\vfs list" | ./kubsh | head -5

# Тест echo
echo "5. Тест команды echo..."
echo 'echo "Test passed"' | ./kubsh

# Тест выхода
echo "6. Тест выхода..."
echo '\q' | ./kubsh

echo "=== Все тесты пройдены ==="
