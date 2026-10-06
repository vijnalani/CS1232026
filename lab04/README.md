# Лабораторна робота № 4

**Помилки, збірка, оптимізація і профілювання: утиліта wordfreq**

Повний текст роботи — у Google Classroom. Тут лише те, що потрібно для здачі.

## Що здається

```
lab04/submissions/ГРУПА_Прізвище/
├── wordfreq.h  scan.c  table_naive.c  Makefile   ← скопіювати з lab04/starter/ без змін
├── main.c          — TODO 1–5: аргументи, файли, помилки, перевірка виводу
├── table_hash.c    — хеш-таблиця з ланцюжками (FNV-1a)
├── buggy.c         — шість виправлень, кожне з коментарем
├── overflow.c      — перевірка переповнення без невизначеної поведінки
└── REPORT.md
```

`make` має зібрати `wordfreq` і `wordfreq-naive` без попереджень з `-Wall -Wextra`.

## Специфікація

```
wordfreq [-n N] [ФАЙЛ...]
```

- без файлів читає `stdin`; друкує `N` (за замовчуванням 10) рядків `КІЛЬКІСТЬ СЛОВО`;
- `N` — ціле від 1 до `INT_MAX`, інакше код `2` і `usage` у `stderr`;
- поганий файл не зупиняє роботу: `wordfreq: ФАЙЛ: <strerror>` у `stderr`, наступний файл;
- помилка запису виводу: `wordfreq: stdout: <strerror>`;
- код повернення: `0` — усе гаразд, `1` — була помилка файла, запису чи пам'яті, `2` — аргументи.

Потрібен Linux (WSL 2, віртуальна машина або Docker з `--privileged`), пакети
`build-essential clang valgrind linux-tools-generic python3`.

## Перевірка й вимірювання

Команди виконуються з вашого каталогу `lab04/submissions/ГРУПА_Прізвище/`.

```bash
python3 ../../../tools/gen-text.py 300000 > corpus-300k.txt   # однаковий у всіх
python3 ../../../tools/gen-text.py 30000  > corpus-30k.txt

make all asan
python3 ../../../tools/wordfreq-test.py ./wordfreq-naive      # має бути 12/12
python3 ../../../tools/wordfreq-test.py ./wordfreq
python3 ../../../tools/wordfreq-test.py ./wordfreq-asan       # і без жодного звіту санітайзера
valgrind --leak-check=full --error-exitcode=9 ./wordfreq -n 3 corpus-30k.txt

make levels
python3 ../../../tools/bench.py -r 3 -- ./wf-O2 corpus-300k.txt
perf record -g ./wordfreq-naive corpus-300k.txt > /dev/null
perf report --stdio --no-children -g none | head -20
```

Програма, що не проходить `wordfreq-test.py`, не вимірюється й не зараховується.

## Що має бути в REPORT.md

Середовище (`uname -r`, процесор, `nproc`, версії `gcc`, `clang`, `valgrind`, `perf`) ·
вивід `wordfreq-test.py` для трьох збірок · таблиця рівнів оптимізації · таблиця
«шість помилок × п'ять способів запуску» · зведена таблиця · відповіді на дев'ять
питань для звіту · висновки.
