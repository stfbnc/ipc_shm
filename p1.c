#include <stdio.h>
#include <errno.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>

#define NAME "/cpshm"

typedef struct shm
{
    pthread_mutex_t mtx;
    int cnt;
} shm;

int main()
{
    int fd = shm_open(NAME, O_CREAT | O_RDWR, S_IRWXU | S_IRWXG | S_IRWXO);
    if(fd == -1)
    {
        fprintf(stderr, "p1 : shm_open(%s): %s (%d)\n", NAME, strerror(errno), errno);
        return 1;
    }

    if(ftruncate(fd , sizeof(shm)))
    {
        fprintf(stderr, "p1 : ftruncate(%s): %s (%d)\n", NAME, strerror(errno), errno);
        return 1;
    }

    shm* shared_memory = (shm*)mmap(NULL, sizeof(shm), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	if((void*)shared_memory == MAP_FAILED)
    {
		fprintf(stderr, "p1 : mmap(%s, %zu): %s (%d)\n", NAME, sizeof(shm), strerror(errno), errno);
        return 1;
	}

	close(fd);

    pthread_mutexattr_t attr;
    if(pthread_mutexattr_init(&attr))
    {
        fprintf(stderr, "p1 : pthread_mutexattr_init\n");
        return 1;
    }
    if(pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED))
    {
        fprintf(stderr, "p1 : pthread_mutexattr_setpshared\n");
        return 1;
    }
    if(pthread_mutexattr_setrobust(&attr, PTHREAD_MUTEX_ROBUST))
    {
        fprintf(stderr, "p1 : pthread_mutexattr_setrobust\n");
        return 1;
    }

    if(pthread_mutex_init(&shared_memory->mtx, &attr))
    {
        fprintf(stderr, "p1 : pthread_mutex_init\n");
        return 1;
    }
    shared_memory->cnt = 0;

    fprintf(stderr, "p1 : Initial counter is %d\n", shared_memory->cnt);
    int i = -1;
    while(++i < 1000)
    {
        if(pthread_mutex_lock(&shared_memory->mtx) != 0) { fprintf(stderr, "p1 : Error locking mutex (loop)\n"); }
        shared_memory->cnt += 1;
        fprintf(stderr, "p1 : Counter is %d\n", shared_memory->cnt);
        if(pthread_mutex_unlock(&shared_memory->mtx) != 0) { fprintf(stderr, "p1 : Error unlocking mutex (loop)\n"); }
        usleep(100000);
    }
    if(pthread_mutex_lock(&shared_memory->mtx) != 0) { fprintf(stderr, "p1 : Error locking mutex\n"); }
    fprintf(stderr, "p1 : Final counter is %d\n", shared_memory->cnt);
    if(pthread_mutex_unlock(&shared_memory->mtx) != 0) { fprintf(stderr, "p1 : Error unlocking mutex\n"); }

    int un_ret = shm_unlink(NAME);

    return 0;
}