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
    int fd = shm_open(NAME, O_RDWR, S_IRWXU | S_IRWXG | S_IRWXO);
    if(fd == -1)
    {
        fprintf(stderr, "p2 : shm_open(%s): %s (%d)\n", NAME, strerror(errno), errno);
        return 1;
    }

    if(ftruncate(fd , sizeof(shm)))
    {
        fprintf(stderr, "p2 : ftruncate(%s): %s (%d)\n", NAME, strerror(errno), errno);
        return 1;
    }

    shm* shared_memory = (shm*)mmap(NULL, sizeof(shm), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	if((void*)shared_memory == MAP_FAILED)
    {
		fprintf(stderr, "p2 : mmap(%s, %zu): %s (%d)\n", NAME, sizeof(shm), strerror(errno), errno);
        return 1;
	}

	close(fd);

    int i = -1;
    while(++i < 2000)
    {
        if(pthread_mutex_lock(&shared_memory->mtx) != 0) { fprintf(stderr, "p2 : Error locking mutex (loop)\n"); }
        shared_memory->cnt += 1;
        fprintf(stderr, "p2 : Counter is %d\n", shared_memory->cnt);
        if(pthread_mutex_unlock(&shared_memory->mtx) != 0) { fprintf(stderr, "p2 : Error unlocking mutex (loop)\n"); }
        usleep(50000);
    }
    if(pthread_mutex_lock(&shared_memory->mtx) != 0) { fprintf(stderr, "p2 : Error locking mutex\n"); }
    fprintf(stderr, "p2 : Final counter is %d\n", shared_memory->cnt);
    if(pthread_mutex_unlock(&shared_memory->mtx) != 0) { fprintf(stderr, "p2 : Error unlocking mutex\n"); }

    return 0;
}