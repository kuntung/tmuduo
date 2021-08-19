#include <stdio.h>
#include <time.h>
#include <pthread.h>
#include <unistd.h>

void prepare(void)
{
    printf("pid = %d prepare ... \n", static_cast<int>(getpid()));

}

void parent(void)
{
    printf("pid = %d parent... \n", static_cast<int>(getpid()));
    
}

void child(void)
{
    printf("pid = %d child... \n", static_cast<int>(getpid()));
}

int main(void)
{
    printf("pid = %d entering main ...\n", static_cast<int>(getpid()));

    pthread_atfork(prepare, parent, child);

    fork();
    sleep(10);
    printf("pid = %d exiting main ...\n", static_cast<int>(getpid()));
    return 0;
}
