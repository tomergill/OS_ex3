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
#include <ctype.h>

#define FIFO_NAME "fifo_clientTOServer"
#define SHM_SIZE 4096 //shared memory size is 4KB (Page Size)
#define ROWS 8
#define COLS 8

#define IS_IN_BOARD_X(col) ((col) >= 0 || (col) < COLS)
#define IS_IN_BOARD_Y(row) ((row) >= 0 || (row) < ROWS)
#define GET_DIGIT_FROM_CHAR(c) ((c) - '0')
#define GET_CHAR_FROM_DIGIT(c) ((c) + '0')
#define OTHER_COLOR(color) (((color) % 2) + 1)

typedef enum {EMPTY = 0, WHITE = 1, BLACK = 2} TILE;



char gotSIGUSR1 = 0;

void SIGUSR1Handler(int flag)
{
    if (flag == SIGUSR1)
        gotSIGUSR1 = 1;
}

void PrintBoard(TILE board[ROWS][COLS])
{
    int i, j;

    printf("The board is:\n");
    for (i = ROWS - 1; i >= 0; --i)
    {
        for (j = 0; j < COLS; ++j) {
            printf("%d", board[i][j]);
        }
        printf("\n");
    }
}

int AddPieceAndFlip(int startX, int startY, int dx, int dy,
                    TILE board[ROWS][COLS], TILE color)
{
    int x, y, counter = 0;
    for (x = startX + dx, y = startY + dy; IS_IN_BOARD_X(x) && IS_IN_BOARD_Y(y);
         ++x, ++y)
    {
        if (board[y][x] == OTHER_COLOR(color))
            continue;
        if (board[y][x] == EMPTY)
            break;

        /* board[y][x] == color */
        for (x -= dx, y -= dy;
             x != startX && y != startY;
             x -= dx, y -= dy, ++counter)
            board[y][x] = color;
        break;
    }

    return counter;
}

int MakeAMove(TILE board[ROWS][COLS], TILE color, int row, int col)
{
    if (row < 0 || row >= ROWS || col < 0 || col >= COLS)
        return -1; //error

    int sum = 0;

    /* flip to every direction and sum the number of flipped pieces */
    sum += AddPieceAndFlip(col, row, -1, -1, board, color); //left-up
    sum += AddPieceAndFlip(col, row, -1, 0, board, color); //left
    sum += AddPieceAndFlip(col, row, -1, 1, board, color); //left-down

    sum += AddPieceAndFlip(col, row, 0, -1, board, color); //up
    sum += AddPieceAndFlip(col, row, 0, 1, board, color); //down

    sum += AddPieceAndFlip(col, row, 1, -1, board, color); //right-up
    sum += AddPieceAndFlip(col, row, 1, 0, board, color); //right
    sum += AddPieceAndFlip(col, row, 1, 1, board, color); //right-down

    return sum;
}

char GetCharFromTileEnum(TILE t)
{
    switch (t)
    {
        case BLACK:
            return 'b';
        case WHITE:
            return 'w';
        default:
            return 'e';
    }
}

void waitForOtherPlayer(char *data, TILE board[ROWS][COLS], TILE color)
{
    do {
        printf("Waiting for the other player to make a move\n");
        if (*data == GetCharFromTileEnum(OTHER_COLOR(color)))
            break;
    } while (!sleep(1));

    int x = GET_DIGIT_FROM_CHAR(data[1]),
            y = GET_DIGIT_FROM_CHAR(data[2]);

    MakeAMove(board, OTHER_COLOR(color), y, x);
    PrintBoard(board);
}

int main()
{
    int fifo, shmid;
    char buffer[16];
    key_t key;
    char *data;
    TILE board[ROWS][COLS], copyBoard[ROWS][COLS];
    int i, j, x, y;
    TILE myColor;
    char gameEnded = 0;
    int piecesFlipped;

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

    /* grab the shared memory created by server: */
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

    /* defining the player's color */
    if (*data == 'b') //aka second player
        myColor = WHITE;
    else
        myColor = BLACK;

    while (!gameEnded)
    {
        if (myColor == WHITE)
            waitForOtherPlayer(data, board, myColor);

        printf("Please choose a square\n");
        x = y = -1;

        do
        {
            scanf("%5s", buffer);
            buffer[5] = '\0';
            if (buffer[0] != '[' || buffer[4] != '[' || buffer[2] != ',' ||
                    !isdigit(buffer[1]) || !isdigit(buffer[3]))
            {
                printf("This square is invalid\n"
                               "Please choose another square\n");
                continue;
            }
            x = GET_DIGIT_FROM_CHAR(buffer[1]);
            y = GET_DIGIT_FROM_CHAR(buffer[3]);
            if (x < 0 || x >= COLS || y < 0 || y >= ROWS)
            {
                printf("No such square\nPlease choose another square\n");
                x = y = -1;
                continue;
            }
            memcpy(copyBoard, board, sizeof(TILE) * ROWS * COLS);
            if (board[y][x] != EMPTY ||
                    (piecesFlipped = MakeAMove(copyBoard, myColor, y, x)) == 0)
            {
                printf("This square is invalid\n"
                               "Please choose another square\n");
                x = y = -1;
                continue;
            }
            memcpy(board, copyBoard, sizeof(TILE) * ROWS * COLS);
        } while (x != -1 && y != -1);

        PrintBoard(board);
        data[0] = GetCharFromTileEnum(myColor);
        data[1] = (char) GET_CHAR_FROM_DIGIT(x);
        data[2] = (char) GET_CHAR_FROM_DIGIT(y);
        data[3] = '\0';

        if (myColor == BLACK)
            waitForOtherPlayer(data, board, myColor);
    }
}