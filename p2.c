#include <stdio.h>
#include <errno.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>

#define NAME "/cpshm"
#define N 1000

typedef struct shm
{
    pthread_mutex_t mtx;
    int error;
    int consumed;
    unsigned char frame[N];
} shm;

int check_arr_vals(unsigned char* arr, unsigned char* val)
{
    int ret = 0;
    for(int i = 1; i < N; i++)
    {
        if(arr[i] != arr[0])
        {
            ret = -1;
            break;
        }
    }
    *val = arr[0];

    return ret;
}

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

    unsigned char old_val = 200;
    int i = -1;
    while(++i < 1000)
    {
        int local_consumed = 1;
        int sleep_cnt = 0;
        int main_process_dead = 0;
        while(local_consumed == 1)
        {
            if(pthread_mutex_lock(&shared_memory->mtx) != 0)
            {
                fprintf(stderr, "p2 : Error locking mutex (new check)\n");
                continue;
            }
            local_consumed = shared_memory->consumed;
            if(pthread_mutex_unlock(&shared_memory->mtx) != 0)
            {
                fprintf(stderr, "p2 : Error unlocking mutex (new check)\n");
                continue;
            }

            usleep(1000);
            if(++sleep_cnt > 60)
            {
                main_process_dead = 1;
                break;
            }
        }

        if(main_process_dead == 1)
        {
            fprintf(stderr, "p2 : Main process dead, terminating\n");
            break;
        }

        int read_ret = 0;
        unsigned char read_val;
        if(pthread_mutex_lock(&shared_memory->mtx) != 0)
        {
            fprintf(stderr, "p2 : Error locking mutex (val check)\n");
            continue;
        }
        read_ret = check_arr_vals(shared_memory->frame, &read_val);
        shared_memory->consumed = 1;
        if(pthread_mutex_unlock(&shared_memory->mtx) != 0)
        {
            fprintf(stderr, "p2 : Error unlocking mutex (val check)\n");
            continue;
        }

        if(read_ret != 0)
        {
            fprintf(stderr, "p2 : Frame values are different\n");
        }
        else
        {
            if(read_val == old_val)
            {
                fprintf(stderr, "p2 : Read the same frame\n");
            }
            old_val = read_val;
            fprintf(stderr, "p2 : Frame value is %d\n", read_val);
        }

        usleep(20000);
    }

    return 0;
}