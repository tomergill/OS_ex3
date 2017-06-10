/*******************************************************************************
 * Student name: Tomer Gill
 * Student: 318459450
 * Course Exercise Group: 01 (CS student, actual group is 89231-03)
 * Exercise name: Exercise 2
*******************************************************************************/

#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/ipc.h>
#include <sys/shm.h>

#define FIFO_NAME "fifo_clientTOServer"
#define SHM_SIZE 4096 //shared memory size is 4KB (Page Size)
#define ROWS 8
#define COLS 8

typedef enum {EMPTY = 0, WHITE = 1, BLACK = 2} TILE;

char gotSIGUSR1 = 0;

void SIGUSR1Handler(int flag)
{
    if (flag == SIGUSR1)
        gotSIGUSR1 = 1;
}

int main()
{
    int fifo, shmid;
    char buffer[16];
    key_t key;
    char *data;
    char player;
    TILE board[ROWS][COLS];
    int i, j;

    /* Opening the fifo */
    if ((fifo = open(FIFO_NAME, O_RDWR | O_TRUNC)) == -1)
    {
        perror("open fifo error");
        exit(EXIT_FAILURE);
    }

    /* writing own pid to fifo */
    buffer[sprintf(buffer, "%d", getpid())] = '\0';
    if (write(fifo, buffer, strlen(buffer)) == -1)
    {
        perror("write pid error");
        if (close(fifo) == -1)
            perror("write error : close error");
        exit(EXIT_FAILURE);
    }
    if (close(fifo) == -1)
        if (close(fifo) == -1)
            perror("write error : close error");

    /* SIGUSR1Handler will handle a SIGUSR1 signal */
    if (signal(SIGUSR1, SIGUSR1Handler) == SIG_ERR)
    {
        perror("signal() error");
        if (close(fifo) == -1)
            perror("signal() error : close error");
        exit(EXIT_FAILURE);
    }

    for (i = 0; i < ROWS; ++i)
        for (j = 0; j < COLS; ++j)
            board[i][j] = EMPTY;
    board[3][3] = board[4][4] = BLACK;
    board[3][4] = board[4][3] = WHITE;

    /* wait for a SIGUSR1 signal */
    do
        pause();
    while (!gotSIGUSR1);

    if ((key = ftok("ex31.c", 'k')) == -1) {
        perror("ftok error");
        exit(EXIT_FAILURE);
    }

    /* grab the shared memory created by ex31.c: */
    if ((shmid = shmget(key, SHM_SIZE, 0)) == -1) {
        perror("shmget error");
        exit(EXIT_FAILURE);
    }

    /* attach to the segment to get a pointer to it: */
    data = shmat(shmid, NULL, 0);
    if (data == (char *)(-1)) {
        perror("shmat error");
        exit(EXIT_FAILURE);
    }

    /* first move */
    if (*data != 'b')
    {

    }
}