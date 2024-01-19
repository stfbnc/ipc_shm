#include <stdio.h>
#include <errno.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>

#define NAME "/cpshm"
#define N 1000000000

typedef struct shm
{
    pthread_mutex_t mtx;
    int error;
    int consumed;
    unsigned char frame[N];
} shm;

shm* shared_memory;

void fill_array(unsigned char* arr, unsigned char val)
{
    fprintf(stderr, "p1 : Filling array with value %d\n", val);
    for(int i = 0; i < N; i++)
    {
        arr[i] = val;
    }
    fprintf(stderr, "p1 : Filled array with value %d\n", val);
}

int init()
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

    shared_memory = (shm*)mmap(NULL, sizeof(shm), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
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
    shared_memory->error = 0;
    shared_memory->consumed = 0;
    fill_array(shared_memory->frame, 0);

    return 0;
}

void* fill_shm(void* param)
{
    unsigned char arr_val = 0;
    int i = -1;
    while(++i < 1000)
    {
        arr_val = (arr_val + 1) % 255;
        if(pthread_mutex_lock(&shared_memory->mtx) != 0)
        {
            fprintf(stderr, "p1 : Error locking mutex\n");
            continue;
        }
        fill_array(shared_memory->frame, arr_val);
        shared_memory->consumed = 0;
        if(pthread_mutex_unlock(&shared_memory->mtx) != 0)
        {
            fprintf(stderr, "p1 : Error unlocking mutex\n");
            continue;
        }
        usleep(30000);
    }

    return NULL;
}

int main()
{
    if(init() != 0)
    {
        fprintf(stderr, "p1 : init error\n");
        return 1;
    }

    pthread_t thread_id;
    int ret = pthread_create(&thread_id, NULL, &fill_shm, NULL);
    if(ret != 0)
    {
        fprintf(stderr, "p1 : pthread_create error\n");
        return 1;
    }

    ret = pthread_join(thread_id, NULL);
    if(ret != 0)
    {
        fprintf(stderr, "p1 : pthread_join error\n");
        return 1;
    }

    int un_ret = shm_unlink(NAME);

    return 0;
}