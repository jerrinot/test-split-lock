#include <stdio.h>
#include <stdlib.h>
#include <stdatomic.h>
#include <time.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>

#define CLOCK_TYPE CLOCK_MONOTONIC
#define CACHE_LINE_SIZE 64u


bool shouldLog(struct timespec* deadline) {
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

int threadCount(int argc, char *argv[]) {
    if (argc < 3) {
        printf("Defaulting to a single thread mode. \n"
               "Pass integer as a second parameter to configure thread count \n");
        return 1;
    }
    char *arg = argv[2];
    char *endPtr;
    int threads = (int)strtol(arg, &endPtr, 10);
    if (*endPtr) {
        printf("Invalid thread count! Pass thread an integer as a 2nd parameter\n");
        exit(-1);
    }
    printf("Using %i threads \n", threads);
    return threads;
}

bool shouldSplit(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Defaulting to cache-aligned mode. "
               "Pass 'split' as a first parameter to enable the lock-splitting mode. "
               "Pass 'align' to explicitly configure the cache-aligned mode !\n");
        return false;
    }
    char *arg = argv[1];
    if (strncmp(arg, "align", strlen("align")) == 0) {
        return false;
    } else if (strncmp(arg, "split", strlen("split")) == 0) {
        return true;
    } else {
        printf("Missing Mode. Either 'aligned' or 'split' as a parameter!\n");
        exit(-1);
    }
}

void *loop(void *arg) {
    int *ptr = arg;
    struct timespec next;
    clock_gettime(CLOCK_TYPE, &next);
    next.tv_sec += 1;
    uint counter = 0;
    for (;;) {
        atomic_fetch_add(ptr, 1);
        if (shouldLog(&next)) {
            printf("Per-thread iteration per second: %u\n", counter);
            next.tv_sec += 1;
            counter = 0;
        }
        counter++;
    }
}

int main(int argc, char *argv[]) {
    int *ptr = malloc(2 * CACHE_LINE_SIZE);
    printf("Address from malloc: %p\n", ptr);

    if (shouldSplit(argc, argv)) {
        //move the pointer to the last byte in the cache line
        ptr = (int *) (((uintptr_t) ptr) | (CACHE_LINE_SIZE - 1));
        printf("Address after adjusting: %p\n", ptr);
    } else {
        printf("Not lock splitting, relying on a compiler to have the address aligned within a single cache line\n");
    }

    int threadsCount = threadCount(argc, argv);
    pthread_t *threads = malloc(threadsCount * sizeof(threadsCount));
    int i;
    for (i = 0; i < threadsCount; ++i)
        pthread_create(&threads[i], NULL, loop, ptr);
    for (;;) {
        sleep(1);
        printf("Global iteration count: %u\n", *ptr);
        *ptr = 0;
    }
}
