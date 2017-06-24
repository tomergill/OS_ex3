/*******************************************************************************
 * Student name: Tomer Gill
 * Student: 318459450
 * Course Exercise Group: 01 (CS student, actual group is 89231-03)
 * Exercise name: Exercise 3
*******************************************************************************/

#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/ipc.h>
#include <sys/shm.h>

#define FIFO_NAME "fifo_clientTOserver"
#define SHM_SIZE 4096 //shared memory size is 4KB (Page Size)
#define ROWS 8
#define COLS 8

#define IS_IN_BOARD_X(col) ((col) >= 0 && (col) < COLS)
#define IS_IN_BOARD_Y(row) ((row) >= 0 && (row) < ROWS)
#define GET_DIGIT_FROM_CHAR(c) ((c) - '0') //1 turns to '1'
#define GET_CHAR_FROM_DIGIT(c) ((c) + '0') //'4' turns to 4
#define OTHER_COLOR(color) (((color) % 2) + 1) //BLACK <=> WHITE

/* represents a color of a tile / player */
typedef enum {EMPTY = 0, WHITE = 1, BLACK = 2} TILE;

char gotSIGUSR1 = 0; //flag represents whether the process got SIGUSR1 already

/******************************************************************************
 * function name: SIGUSR1Handler
 * The Input: The signal that activated this function.
 * The output: Turns the flag that indicates SIGUSR1 was handled.
 * The Function operation: Quite literallly.
*******************************************************************************/
void SIGUSR1Handler(int flag)
{
    if (flag == SIGUSR1)
        gotSIGUSR1 = 1;
}

/******************************************************************************
 * function name: PrintBoard
 * The Input: The board to print.
 * The output: Prints the board.
 * The Function operation: Goes over the board and prints it.
*******************************************************************************/
void PrintBoard(TILE board[ROWS][COLS])
{
    int i, j;

    printf("The board is:\n");
    for (i = 0; i < ROWS; ++i)
    {
        for (j = 0; j < COLS; ++j) {
            printf("%d", board[i][j]);
        }
        printf("\n");
    }
}

/******************************************************************************
 * function name: AddPieceAndFlip
 * The Input: The position of the piece that was just put, the direction (as
 *      change in x and y), the board (after piece was put!) and the color of
 *      the player's pieces.
 * The output: The number of pieces flipped as a result of putting the new
 *      piece.
 * The Function operation: Goes from the new piece to the direction
 *      specified. As long as it still within the board's borders and the color
 *      of the pieces are the other's color, it continues in that direction.
 *      If there isn't a piece in our color and it's the edge of the board - no
 *      pieces was flipped so 0 is returned.
 *      If found an empty tile with no piece on it that means the move isn't
 *      legal and so it returns that 0 pieces were flipped.
 *      If found a piece with matching color, it goes back and flip every piece
 *      in the other color to our color, and then returns how many were
 *      flipped (0 means that a piece was put next to another piece of the
 *      same color, meaning no pieces were flipped and that this is an
 *      illegal move).
*******************************************************************************/
int AddPieceAndFlip(int startX, int startY, int dx, int dy,
                    TILE board[ROWS][COLS], TILE color)
{
    int x, y, counter = 0;
    for (x = startX + dx, y = startY + dy;
         IS_IN_BOARD_X(x) && IS_IN_BOARD_Y(y);
         x += dx, y+=dy)
    {
        if (board[y][x] == OTHER_COLOR(color))
            continue;
        if (board[y][x] == EMPTY)
            break;

        /* board[y][x] == color */
        for (x -= dx, y -= dy;
             x != startX || y != startY;
             x -= dx, y -= dy, ++counter)
            board[y][x] = color;
        break;
    }

    return counter;
}

/******************************************************************************
 * function name: MakeAMove
 * The Input: The board, the color of the player making the move and the
 *      position the new piece will be put.
 * The output: The number of pieces flipped as a result of putting the new
 *      piece.
 * The Function operation: Checks if move is within borders, put the new
 *      piece in the board and then activates the AddPieceAndFlip function in
 *      every direction possible, summing the number of pieces flipped as a
 *      result of the operation.
 *      return value of 0 indiciates the move was illegal.
*******************************************************************************/
int MakeAMove(TILE board[ROWS][COLS], TILE color, int row, int col)
{
    if (row < 0 || row >= ROWS || col < 0 || col >= COLS)
        return -1; //error

    int sum = 0;

    board[row][col] = color; //putting piece

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

/******************************************************************************
 * function name: GetCharFromTileEnum
 * The Input: A TILE enum.
 * The output: The char representing the enum.
 * The Function operation: returns the small version of the first letter of
 *      the TILE enum.
*******************************************************************************/
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

/******************************************************************************
 * function name: CheckGameEnd
 * The Input: The board, pointer to the shared memory and a flag indicating
 *      whether this is my turn or not.
 * The output: If the game has ended (due to a full board) the program exits.
 * The Function operation: Goes over all the board, counting how many tiles
 *      of each color there are. If an empty tile is found the func returns.
 *      If board is full, prints the right message and exits the program
 *      after detaching from the shared memory.
 *      If this check wasn't on the player's turn it also write that the game
 *      was finished and the winner to the shared memory for the server.
*******************************************************************************/
void CheckGameEnd(TILE board[ROWS][COLS], char *data, char myTurn)
{
    int i, j, bcounter = 0, wcounter = 0;
    TILE c;

    for (i = 0; i < ROWS; ++i)
    {
        for (j = 0; j < COLS; ++j) {
            switch (board[i][j])
            {
                case BLACK:
                    bcounter++;
                    break;
                case WHITE:
                    wcounter++;
                    break;
                default: //EMPTY
                    return; //game didn't end
            }
        }
    }

    /* If got to this place, the board is full and the game is over */
    if (bcounter > wcounter) {
        printf("Winning player: Black\n");
        c = BLACK;
    }
    else if (wcounter > bcounter) {
        printf("Winning player: White\n");
        c = WHITE;
    }
    else {
        printf("No winning player\n");
        c = EMPTY;
    }

    /*
     * writing to shared memory that the game ended if I wasn't the one who
     * played last turn.
     */
    if (!myTurn) {
        data[2] = '\0';
        data[1] = GetCharFromTileEnum(c);
        data[0] = '*';
    }

    //detaching from shared memory
    if (shmdt(data) == -1) {
        perror("shared memory detach error");
        exit(EXIT_FAILURE);
    }

    exit(EXIT_SUCCESS);
}

/******************************************************************************
 * function name: WaitForOtherPlayer
 * The Input: A pointer to the shared memory, the board and the player's color.
 * The output: Makes the move gotten and finishes the game if it was it's end.
 * The Function operation: Waits for the first char pointed by data to be
 *      either the other's color's char or '*'.
 *      If found the other's char it makes the move the other wrote to the
 *      shared memory, prints the board after the move and checks if the game
 *      is over (if it does, finishes it).
 *      If found the end game char ('*') it means the other player doesn't
 *      have a move to make so the game is over.
*******************************************************************************/
void WaitForOtherPlayer(char *data, TILE board[ROWS][COLS], TILE color)
{
    do {
        printf("Waiting for the other player to make a move\n");
        if (*data == GetCharFromTileEnum(OTHER_COLOR(color)))
            break;
        else if (*data == '*') //game is done
        {
            switch(data[1]) { //check which player won
                default:
                    printf("No winning player\n");
                    break;
                case 'b':
                    printf("Winning player: Black\n");
                    break;
                case 'w':
                    printf("Winning player: White\n");
                    break;
            }

            if (shmdt(data) == -1) {
                perror("shared memory detach error");
                exit(EXIT_FAILURE);
            }

            exit(EXIT_SUCCESS);
        }
    } while (!sleep(1));

    int x = GET_DIGIT_FROM_CHAR(data[1]),
            y = GET_DIGIT_FROM_CHAR(data[2]);

    MakeAMove(board, OTHER_COLOR(color), y, x);
    PrintBoard(board);
    CheckGameEnd(board, data, 0);
}

/******************************************************************************
 * function name: CheckForPossibleMoves
 * The Input: The board, a pointer to the shared memory, and the player's color.
 * The output: Check if there is a move to make. If not, ends the game and
 *      writes who won to the shared memory.
 * The Function operation: Goes over every empty tile, putting a piece there
 *      and checking if this is a legal move, if so returns. If there aren't
 *      moves to make the game is closed and the ending is writtrn to the
 *      shared memory with the game finished char '*'.
*******************************************************************************/
void CheckForPossibleMoves(TILE board[ROWS][COLS], char *data, TILE color)
{
    int i, j, bcounter = 0, wcounter = 0;
    TILE copyBoard[ROWS][COLS], c;

    for (i = 0; i < ROWS; ++i)
    {
        for (j = 0; j < COLS; ++j) {
            if (board[i][j] != EMPTY) {
                if (board[i][j] == BLACK)
                    bcounter++;
                else //WHITE
                    wcounter++;
                continue;
            }
            memcpy(copyBoard, board, sizeof(TILE) * ROWS * COLS);
            if (MakeAMove(copyBoard, color, i, j)) //means there is a valid move
                return;
        }
    }

    /* There isn't a move to be made - check who won and end the game */

    /* If got to this place, the board is full and the game is over */
    if (bcounter > wcounter) {
        printf("Winning player: Black\n");
        c = BLACK;
    }
    else if (wcounter > bcounter) {
        printf("Winning player: White\n");
        c = WHITE;
    }
    else {
        printf("No winning player\n");
        c = EMPTY;
    }

    /*
     * writing to shared memory that the game ended.
     */
        data[2] = '\0';
        data[1] = GetCharFromTileEnum(c);
        data[0] = '*';

    if (shmdt(data) == -1) {
        perror("shared memory detach error");
        exit(EXIT_FAILURE);
    }

    exit(EXIT_SUCCESS);
}

/******************************************************************************
 * function name: main
 * The Input: None.
 * The output: Reversi game.
 * The Function operation: Opens the fifo, writes it pid to it, and then wait
 *      to get SIGUSR1 from the server. Then finds out which color it is, and
 *      the makes a move and waits to other player to make a move, until the
 *      end of the game.
*******************************************************************************/
int main()
{
    int fifo, shmid;
    char buffer[16];
    key_t key;
    char *data;
    TILE board[ROWS][COLS], copyBoard[ROWS][COLS];
    int i, j, x, y;
    TILE myColor;

    /* Opening the fifo */
    if ((fifo = open(FIFO_NAME, O_RDWR)) == -1)
    {
        perror("open fifo error");
        exit(EXIT_FAILURE);
    }

    /* writing own pid to fifo */
    pid_t mypid = getpid();
    if (write(fifo, &mypid, sizeof(pid_t)) == -1)
    {
        perror("write pid error");
        if (close(fifo) == -1)
            perror("write error : close fifo error");
        exit(EXIT_FAILURE);
    }
    if (close(fifo) == -1)
        perror("after write : close error");

    /* SIGUSR1Handler will handle a SIGUSR1 signal */
    if (signal(SIGUSR1, SIGUSR1Handler) == SIG_ERR)
    {
        perror("signal() error");
        if (close(fifo) == -1)
            perror("signal() error : close error");
        exit(EXIT_FAILURE);
    }

    /* initializing the board */
    for (i = 0; i < ROWS; ++i)
        for (j = 0; j < COLS; ++j)
            board[i][j] = EMPTY;
    board[3][3] = board[4][4] = BLACK;
    board[3][4] = board[4][3] = WHITE;


    /* wait for a SIGUSR1 signal */
    do
        pause();
    while (!gotSIGUSR1);

    /* get key to shared memory */
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
    else { //first player
        myColor = BLACK;
        //*data = 'b';
        PrintBoard(board);
    }

    while (1)
    {
        if (myColor == WHITE)
            WaitForOtherPlayer(data, board, myColor);

        CheckForPossibleMoves(board, data, myColor);

        printf("Please choose a square\n");
        x = y = -1;

        do
        {
            scanf("%15s", buffer);
            sscanf(buffer, "[%d,%d]", &x, &y);

            if (x == -1 || y == -1 || strchr(buffer, '[') == NULL ||
                    strchr(buffer, ']') == NULL || strchr(buffer, ',') == NULL)
            {
                printf("This square is invalid\n"
                               "Please choose another square\n");
                x = y = -1;
                continue;
            }
            if (x < 0 || x >= COLS || y < 0 || y >= ROWS)
            {
                printf("No such square\nPlease choose another square\n");
                x = y = -1;
                continue;
            }

            /* try to make a move */
            memcpy(copyBoard, board, sizeof(TILE) * ROWS * COLS);
            if (board[y][x] != EMPTY ||
                    MakeAMove(copyBoard, myColor, y, x) == 0)
            {
                printf("This square is invalid\n"
                               "Please choose another square\n");
                x = y = -1;
                continue;
            }

            /* if move was legal make the temp board the main board */
            memcpy(board, copyBoard, sizeof(TILE) * ROWS * COLS);

        } while (x == -1 || y == -1);

        PrintBoard(board);

        data[3] = '\0'; //null-terminator
        data[2] = (char) GET_CHAR_FROM_DIGIT(y); //row number
        data[1] = (char) GET_CHAR_FROM_DIGIT(x); //column number
        data[0] = GetCharFromTileEnum(myColor); //color = b / w

        CheckGameEnd(board, data, 1);

        if (myColor == BLACK)
            WaitForOtherPlayer(data, board, myColor);
    }
}