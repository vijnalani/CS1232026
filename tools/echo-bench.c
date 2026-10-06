/* Навантажувальний клієнт для ЛР 4.
 *
 *   echo-bench -p ПОРТ [-c З'ЄДНАНЬ] [-d СЕКУНД] [-s РОЗМІР] [-i ПРОСТОЮЧИХ]
 *
 * -c  активні з'єднання: кожне в окремому потоці шле повідомлення й чекає на відповідь
 * -i  додаткові з'єднання, які лише тримаються відкритими (імітація «повільних» клієнтів)
 * Друкує кількість обмінів, обміни/с і затримку (медіана, 99-й процентиль).
 * Збірка: make -C tools
 */
#define _GNU_SOURCE
#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>

static int port = 0, nconn = 10, seconds = 5, msg = 64, idle = 0;
static volatile int stop = 0;

struct worker { pthread_t th; long count; double *lat; long cap; int failed; };

static double now_us(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1e6 + ts.tv_nsec / 1e3;
}

static int dial(void)
{
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in a = { .sin_family = AF_INET, .sin_port = htons(port),
                             .sin_addr.s_addr = htonl(INADDR_LOOPBACK) };
    if (connect(fd, (struct sockaddr *)&a, sizeof a) < 0) { close(fd); return -1; }
    int one = 1;
    setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &one, sizeof one);
    return fd;
}

static void *run(void *arg)
{
    struct worker *w = arg;
    int fd = dial();
    if (fd < 0) { w->failed = 1; return NULL; }
    char *out = malloc(msg), *in = malloc(msg);
    memset(out, 'x', msg);
    while (!stop) {
        double t0 = now_us();
        if (write(fd, out, msg) != msg) { w->failed = 1; break; }
        int got = 0;
        while (got < msg) {
            ssize_t n = read(fd, in + got, msg - got);
            if (n <= 0) { w->failed = 1; goto done; }
            got += n;
        }
        if (w->count == w->cap) {
            w->cap = w->cap ? w->cap * 2 : 4096;
            w->lat = realloc(w->lat, w->cap * sizeof *w->lat);
        }
        w->lat[w->count++] = now_us() - t0;
    }
done:
    close(fd);
    free(out); free(in);
    return NULL;
}

static int cmp(const void *a, const void *b)
{
    double x = *(const double *)a, y = *(const double *)b;
    return (x > y) - (x < y);
}

int main(int argc, char **argv)
{
    int opt;
    while ((opt = getopt(argc, argv, "p:c:d:s:i:")) != -1) {
        switch (opt) {
        case 'p': port = atoi(optarg); break;
        case 'c': nconn = atoi(optarg); break;
        case 'd': seconds = atoi(optarg); break;
        case 's': msg = atoi(optarg); break;
        case 'i': idle = atoi(optarg); break;
        default: goto usage;
        }
    }
    if (port <= 0 || nconn <= 0 || seconds <= 0 || msg <= 0 || idle < 0) {
usage:
        fprintf(stderr, "використання: %s -p ПОРТ [-c 10] [-d 5] [-s 64] [-i 0]\n", argv[0]);
        return 2;
    }

    int *idle_fds = calloc(idle ? idle : 1, sizeof *idle_fds);
    for (int i = 0; i < idle; i++) {
        if ((idle_fds[i] = dial()) < 0) {
            fprintf(stderr, "не вдалося відкрити простоююче з'єднання №%d: %s\n", i, strerror(errno));
            return 1;
        }
    }

    struct worker *ws = calloc(nconn, sizeof *ws);
    for (int i = 0; i < nconn; i++) pthread_create(&ws[i].th, NULL, run, &ws[i]);
    sleep(seconds);
    stop = 1;

    long total = 0; int failed = 0;
    for (int i = 0; i < nconn; i++) { pthread_join(ws[i].th, NULL); total += ws[i].count; failed += ws[i].failed; }

    double *all = malloc((total ? total : 1) * sizeof *all);
    long k = 0;
    for (int i = 0; i < nconn; i++) { memcpy(all + k, ws[i].lat, ws[i].count * sizeof *all); k += ws[i].count; }
    qsort(all, total, sizeof *all, cmp);

    printf("з'єднань: %d активних + %d простоюючих, повідомлення %d Б, %d с\n", nconn, idle, msg, seconds);
    printf("обмінів:  %ld (%.0f/с)\n", total, total / (double)seconds);
    if (total)
        printf("затримка: медіана %.1f мкс, p99 %.1f мкс\n", all[total / 2], all[(long)(total * 0.99)]);
    if (failed) printf("УВАГА: %d з'єднань обірвалися з помилкою\n", failed);

    for (int i = 0; i < idle; i++) close(idle_fds[i]);
    return failed ? 1 : 0;
}
