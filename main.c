#include <stdio.h>
#include <stdlib.h>
#include <stdatomic.h>
#include <time.h>
#include <stdbool.h>
#include <stdint.h>

#define CLOCK_TYPE CLOCK_MONOTONIC
#define CACHE_LINE_SIZE 64u

bool isDeadlineReached(struct timespec* deadline) {
    struct timespec currentTime;
    clock_gettime(CLOCK_TYPE, &currentTime);
    if (currentTime.tv_sec < deadline->tv_sec) {
        return false;
    }
    if (currentTime.tv_sec > deadline->tv_sec) {
        return true;
    }
    return currentTime.tv_nsec >= deadline->tv_nsec;
}

int main(int argc, char *argv[]) {
    uintptr_t buff = (uintptr_t)malloc(2 * CACHE_LINE_SIZE);
    printf("Address from malloc: %lu\n", buff);

    //move the pointer to the last byte in the cache line
    buff = buff | (CACHE_LINE_SIZE - 1);

    int *ptr = (int *) buff;
    printf("Address after adjusting: %p\n", ptr);

    struct timespec next;
    clock_gettime(CLOCK_TYPE, &next);
    next.tv_sec += 1;

    unsigned int counter = 0;
    for (;;) {
        atomic_fetch_add (ptr, 1);
        if (isDeadlineReached(&next)) {
            printf("Iteration per second: %i\n", counter);
            printf("Address: %ls\n", ptr);
            next.tv_sec += 1;
            counter = 0;
        }
        counter++;
    }
}
