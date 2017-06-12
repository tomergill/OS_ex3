/*******************************************************************************
 * Student name: Tomer Gill
 * Student: 318459450
 * Course Exercise Group: 01 (CS student, actual group is 89231-03)
 * Exercise name: Exercise 3
*******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>


#define SHM_SIZE 4096 //shared memory size is 4KB (Page Size)
#define FIFO_NAME "fifo_clientTOServer" //name of the fifo "file"

/******************************************************************************
 * function name: GetPidFromFifo
 * The Input: The fifo file descriptor, and a pointer to a pid_t struct to be
 * filled with a pid.
 * The output: Either a pid_t from the fifo, or NULL if there were a problem
 * with the read from the fifo.
 * The Function operation: Reads from the fifo the struct.
*******************************************************************************/
void GetPidFromFifo(int fifoFd, pid_t *pid)
{
    int bytesRead;

    if ((bytesRead = read(fifoFd, pid, sizeof(pid_t))) == -1)
    {
        perror("error reading pid");
        unlink(FIFO_NAME);
        exit(EXIT_FAILURE);
    }
    else if (bytesRead != sizeof(pid_t))
    {
        perror("error reading pid");
        unlink(FIFO_NAME);
        exit(EXIT_FAILURE);
    }
    //read into pid_t
}

/******************************************************************************
 * function name: main
 * The Input: None.
 * The output: Reversi game.
 * The Function operation: Creates a fifo, then reads from it 2 pids of
 * client processes that will play the game. Then it closes the fifo, creates
 * a shared memory for the clients to play and then signals them a SIGUSR1
 * that notifies them that they can start playing.
*******************************************************************************/
int main() {
    key_t key;
    int fifo;
    pid_t pid1, pid2;
    char *data;
    int shmid;

    //creating a key
    if ((key = ftok("ex31.c", 'k')) == -1) {
        perror("server ftok error");
        exit(EXIT_FAILURE);
    }

    /* connect to (and possibly create) the segment: */
    if ((shmid = shmget(key, SHM_SIZE, 0644 | IPC_CREAT)) == -1) {
        perror("server shmget error");
        exit(EXIT_FAILURE);
    }

    /* attach to the segment to get a pointer to it: */
    data = shmat(shmid, NULL, 0);
    if (data == (char *)(-1)) {
        perror("server shmat error");
        exit(EXIT_FAILURE);
    }

    //creating fifo
    if (mkfifo(FIFO_NAME, 0666) == -1)
    {
        perror("server mkfifo error");
        exit(EXIT_FAILURE);
    }

    if ((fifo = open(FIFO_NAME, O_RDWR | O_TRUNC)) == -1)
    {
        perror("server open fifo error");
        unlink(FIFO_NAME);
        exit(EXIT_FAILURE);
    }

    GetPidFromFifo(fifo, &pid1);
    GetPidFromFifo(fifo, &pid2);

    if (close(fifo) == -1)
        perror("error closing fifo fd");
    if (unlink(FIFO_NAME) == -1)
        perror("error deleting fifo");

    kill(pid1, SIGUSR1);
    while (*data != 'b') {sleep(1);} //TODO change this to what move is
    kill(pid2, SIGUSR1);

    while (*data != 'w') {sleep(1);} //TODO change this to win condition

    printf("GAME OVER\n");
    //TODO print appropriate error message.

    return EXIT_SUCCESS;
}