# Інструменти, спільні для всіх робіт курсу.
CFLAGS ?= -O2 -Wall -Wextra

echo-bench: echo-bench.c
	$(CC) $(CFLAGS) -pthread -o $@ $<

clean:
	rm -f echo-bench

.PHONY: clean
