# Лабораторна робота № 5

**Процеси: життєвий цикл, демонізація, віртуальна пам'ять і нелокальні стрибки**

Повний текст роботи — у Google Classroom. Тут лише те, що потрібно для здачі.

## Що здається

```
lab05/submissions/ГРУПА_Прізвище/
├── Makefile        ← скопіювати з lab05/starter/ без змін
├── selfinfo.c      — TODO 1: region_of за /proc/self/maps
├── launch.c        — TODO 2a, 2b (запуск), TODO 3 (демон) і одна вада поза TODO
├── forkcost.c      — TODO 4: ціна fork і copy-on-write
├── calc.c          — дві вади з setjmp/longjmp і TODO 5 (переповнення)
└── REPORT.md
```

`make` має зібрати `selfinfo`, `launch`, `forkcost` і `calc` без жодного попередження з `-Wall -Wextra`.

## Специфікація launch

```
launch [-e ІМ'Я=ЗНАЧЕННЯ]... [-u ІМ'Я]... [-C КАТАЛОГ] [-o ФАЙЛ]
       [-l ЖУРНАЛ] [-d [-p PIDФАЙЛ]] [--] ПРОГРАМА [АРГ...]
```

- аргументи передаються точно; програма шукається в `PATH`; спершу всі `-u`, потім усі `-e`;
- дитина не успадковує жодного дескриптора, крім 0, 1, 2; сам `launch` нічого не друкує в `stdout`;
- коди: код дитини · `128+N` і `launch: ПРОГРАМА: killed by signal N (опис)` · `127` — не знайдено ·
  `126` — не виконується · `1` — не вдалися `-C`/`-o` · `2` — аргументи; помилки — `launch: ОБ'ЄКТ: strerror`;
- `-l`: рядки `start pid=P cmd=…`, `exit pid=P code=C`, `signal pid=P sig=N`, `fail …`, `daemon pid=P cmd=…`;
- `-d`: подвійний `fork`, `setsid`, `umask 022`, `cwd /` (якщо немає `-C`), stdin з `/dev/null`,
  stdout і stderr — у `-o` або `/dev/null`; повертається одразу; PID-файл — лише якщо `exec` вдався.

Потрібен Linux (WSL 2, віртуальна машина або Docker), пакети
`build-essential clang valgrind strace python3 procps psmisc`.

## Перевірка й вимірювання

Команди виконуються з вашого каталогу `lab05/submissions/ГРУПА_Прізвище/`.

```bash
make
python3 ../../../tools/lab05-test.py .                 # має бути 17/17

./selfinfo -m | grep '^addr'
./launch -d -p d.pid -o d.out -- "$PWD/selfinfo" -s 60
ps -o pid,ppid,pgid,sid,tty,stat,cmd -p "$(cat d.pid)"; kill "$(cat d.pid)"

./forkcost -r 7 0 64 256 1024

printf '1+2\n2*(3+4)\n1/0\n5-\n10/3\n(1\n7\n' > in.txt
make calc-variants
for b in calc-gcc-O0 calc-gcc-O2 calc-clang-O2; do ./$b < in.txt | tail -1; done
valgrind --leak-check=full ./calc < in.txt > /dev/null
```

Програма `launch`, що не проходить тести частини 2 (`t_exit_code` … `t_stdio_once`), не зараховується.

## Що має бути в REPORT.md

Середовище (`uname -r`, процесор, `nproc`, версії `gcc`, `clang`, `valgrind`,
`transparent_hugepage/enabled`) · вивід `lab05-test.py` · таблиця `forkcost` ·
зведена таблиця · відповіді на дев'ять питань для звіту · висновки.
