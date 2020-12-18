#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>
#include <wchar.h>

#include "snake.h"
#include "enums.h"

struct termios old_mode;


void set_new_terminal_mode() {
    tcgetattr(STDIN_FILENO, &old_mode); //STDIN_FILENO == fileno(stdin)

    setbuf(stdout, NULL);

    struct termios mode;
    mode = old_mode;

    mode.c_lflag &= ~(ICANON | ECHO); //turning off canonical mode and echo
    mode.c_cc[VMIN] = 0; //setting 0 to minimal input character amount
    mode.c_cc[VTIME] = 0; //setting time to 0 of waiting of putting next character


    tcsetattr(STDIN_FILENO, TCSANOW, &mode);

    system("clear"); //for clearing any text in terminal
    printf("\x1b[?25l"); //for hiding mouse
}

void set_old_terminal_mode() {
    tcsetattr(STDIN_FILENO, TCSANOW, &old_mode);
    printf("\x1b[?25h");
}

//x from top to bottom, y from left to right (like in matrix)





typedef struct food_list {
    int x;
    int y;
    struct food_list *next;
} foodl;

typedef struct field {
    int size;
    int expected_food_amo;
    int cur_food_amo;
    foodl *food;
    char **field_table;
    char ***colors;
} field;





field* init_field(int size, int food_amo) {
    field *fld = (field*)malloc(sizeof(field));

    fld->size = size;

    fld->expected_food_amo = food_amo;

    fld->cur_food_amo = 0;

    fld->food = NULL;

    fld->field_table = (char**)malloc(sizeof(char*) * size);
    for (int i = 0; i < size; i++) {
        fld->field_table[i] = (char*)malloc(sizeof(char) * size);
        for (int j = 0; j < size; j++) {
            fld->field_table[i][j] = ' ';
        }
    }

    fld->colors = (char***)malloc(sizeof(char**) * size);
    for (int i = 0; i < size; i++) {
        fld->colors[i] = (char**)malloc(sizeof(char*) * size);
        for (int j = 0; j < size; j++) {
            fld->colors[i][j] = (char*)malloc(sizeof(char) * 7);
            fld->colors[i][j] = "\e[0m"; //setting default text type
        }
    }

    return fld;
}


void printc(char *string, enum color col) {
	switch (col) {
		case RED:
			printf("\e[31m");
			printf("%s", string);
			printf("\e[0m");
			break;
		case GREEN:
			printf("\e[32m");
			printf("%s", string);
			printf("\e[0m");
			break;
		case YELLOW:
			printf("\e[33m");
			printf("%s", string);
			printf("\e[0m");
			break;
		case BLUE:
			printf("\e[34m");
			printf("%s", string);
			printf("\e[0m");
			break;
		case MAGENTA:
			printf("\e[35m");
			printf("%s", string);
			printf("\e[0m");
			break;
		case CYAN:
			printf("\e[36m");
			printf("%s", string);
			printf("\e[0m");
			break;
		default:
			break;
	}
}

void draw2(field *fld) {
    // moving to 0,0 position on terminal
    printf("\x1b[H");
    //drawing upper border
    // printf("\e[0m");
    printf("┌");
    for (int i = 0; i < fld->size * 2 - 1; i++) {
        // printf("\e[0m");
        printf("─");
    }
    // printf("\e[0m");
    printf("┐");
    putchar('\n');

    //drawing field with side borders
    for (int i = 0; i < fld->size; i++) {
        printf("│");

        // printf("%s", fld->colors[i][0]);
        // putchar(fld->field_table[i][0]);
        printc(&fld->field_table[i][0], fld->colors[i][0]);
        for (int j = 1; j < fld->size; j++) {
            putchar(' ');
            // printf("%s", fld->colors[i][j]);
            // putchar(fld->field_table[i][j]);
            printc(&fld->field_table[i][j], fld->colors[i][j]);
        }

        // printf("\e[0m");
        printf("│");
        putchar('\n');
    }
    //drawing lower border
    // printf("\e[0m");
    printf("└");
    for (int i = 0; i < fld->size * 2 - 1; i++) {
        // printf("\e[0m");
        printf("─");
    }
    // printf("\e[0m");
    printf("┘");
    putchar('\n');
}

void draw(field *fld) {
    // moving to 0,0 position on terminal
    printf("\x1b[H");
    //drawing upper border
    printf("\e[0m");
    printf("┌");
    for (int i = 0; i < fld->size * 2 - 1; i++) {
        printf("\e[0m");
        printf("─");
    }
    printf("\e[0m");
    printf("┐");
    putchar('\n');

    //drawing field with side borders
    for (int i = 0; i < fld->size; i++) {
        printf("│");

        printf("%s", fld->colors[i][0]);
        putchar(fld->field_table[i][0]);
        for (int j = 1; j < fld->size; j++) {
            putchar(' ');
            printf("%s", fld->colors[i][j]);
            putchar(fld->field_table[i][j]);
        }

        printf("\e[0m");
        printf("│");
        putchar('\n');
    }
    //drawing lower border
    printf("\e[0m");
    printf("└");
    for (int i = 0; i < fld->size * 2 - 1; i++) {
        printf("\e[0m");
        printf("─");
    }
    printf("\e[0m");
    printf("┘");
    putchar('\n');
}

int get_cell_type(field *fld, int x, int y) {
    if ((x < 0 || fld->size <= x) || (y < 0 || fld->size <= y)) {
        return -1; //out of field
    }
    switch (fld->field_table[x][y]) {
        case ' ':
            return EMPTY;
        case '$':
            return FOOD;
        case '#':
            return WALL;
        case '@':
            return SNAKE;
        default:
            return -1;
    }
}

void spawn_food(field *fld) {
    if (fld->cur_food_amo >= fld->expected_food_amo) return;

    int x, y;
    do {
        x = rand() % fld->size;
        y = rand() % fld->size;
    } while (get_cell_type(fld, x, y) != EMPTY);

    foodl *food = (foodl*)malloc(sizeof(foodl));
    food->x = x;
    food->y = y;
    food->next = fld->food;
    fld->food = food;

    fld->cur_food_amo++;
}

void eat_food(field *fld, int x, int y) {
    for (foodl **food = &(fld->food); *food != NULL; ) {
        if ((*food)->x == x && (*food)->y == y) {
            foodl *tmp = *food;
            *food = (*food)->next;
            free(tmp);
        }
        else {
            food = &((*food)->next);
        }
    }
    fld->cur_food_amo--;
}


int control1(snake *snk) {
    //try to return type of next cell and add enum of cells' types
    //try to add any useful action/function in cases
    char direction;
    read(STDIN_FILENO, &direction, 1);
    switch (direction) {
        case 'a':
            if (snk->direction != RIGHT) {
                snk->direction = LEFT;
            }
            break;
        case 'd':
            if (snk->direction != LEFT) {
                snk->direction = RIGHT;
            }
            break;
        case 's':
            if (snk->direction != UP) {
                snk->direction = DOWN;
            }
            break;
        case 'w':
            if (snk->direction != DOWN) {
                snk->direction = UP;
            }
            break;
        default:
            return 0;
    }
    return 1;
}

int get_next_cell_coord(snake *snk, int *x, int *y) {
    *x = snk->head->x;
    *y = snk->head->y;
    switch (snk->direction) {
        case LEFT:
            (*y)--;
            break;
        case RIGHT:
            (*y)++;
            break;
        case UP:
            (*x)--;
            break;
        case DOWN:
            (*x)++;
            break;
    }
}

void grow_snake(snake *snk) {
    body *prev_head = snk->head;
    prev_head->body_char = '#';

    body *new_head = (body*)malloc(sizeof(body));
    new_head->color = "\e[31m";
    new_head->body_char = '@';
    get_next_cell_coord(snk, &(new_head->x), &(new_head->y));

    new_head->next = prev_head;
    prev_head->prev = new_head;

    snk->head = new_head;
}

void move(snake *snk) {
    body *prev_head = snk->head;
    prev_head->body_char = '#';

    body *new_head = (body*)malloc(sizeof(body));
    new_head->color = "\e[31m";
    new_head->body_char = '@';
    get_next_cell_coord(snk, &(new_head->x), &(new_head->y));

    new_head->next = prev_head;
    prev_head->prev = new_head;

    snk->head = new_head;

    body *prev_tail = snk->tail;

    snk->tail = snk->tail->prev;
    snk->tail->next = NULL;

    free(prev_tail);
}



//returns 0 if game over otherwise something else
int action(snake *snk, field *fld) {
    //getting coords of next cell
    int x, y;
    get_next_cell_coord(snk, &x, &y);
    switch (get_cell_type(fld, x, y)) {
        case EMPTY:
            move(snk);
            return 1;
        case FOOD:
            grow_snake(snk);
            eat_food(fld, x, y);
            return 2;
        case SNAKE:
            return -1;
        case WALL:
            return -2;
        default:
            return -3;
    }
}

void clear_field(field *fld) {
    for (int i = 0; i < fld->size; i++) {
        for (int j = 0; j < fld->size; j++) {
            fld->colors[i][j] = "\e[0m";
            fld->field_table[i][j] = ' ';
        }
    }
}

void compose(snake *snk, field *fld) {
    int snk_x;
    int snk_y;
    for (body *seg = snk->head; seg != NULL; seg = seg->next) {
        snk_x = seg->x;
        snk_y = seg->y;
        fld->field_table[snk_x][snk_y] = seg->body_char;
        fld->colors[snk_x][snk_y] = seg->color;
    }
    for (foodl *food = fld->food; food != NULL; food = food->next) {
        fld->field_table[food->x][food->y] = '$';
    }
}


int main() {

    //add some fun with figlet (in terminal)
    set_new_terminal_mode();

    //try this
    // atexit(set_old_terminal_mode);

    int field_size = 25;
    int food_amo = 15;
    field *fld = init_field(field_size, food_amo);
    snake *snk = init_snake(fld->size / 2, fld->size / 2);

    struct timespec time;
    clock_gettime(CLOCK_REALTIME, &time);

    int move = 0;
    int game_status = 1;
    int delay = 300 * 1000000; // in nanoseconds
    int delta_time = 0;
    int prev_frame = (int)(time.tv_nsec + time.tv_sec * 1000000000); // in nanoseconds

    while (game_status > 0) {
        move = control1(snk);

        clock_gettime(CLOCK_REALTIME, &time);
        delta_time = (int)(time.tv_nsec + time.tv_sec * 1000000000) - prev_frame;
        //printf("%f\n", (float)delta_time / 1000000000);
        if (move || delta_time > delay) {
            clock_gettime(CLOCK_REALTIME, &time);
            prev_frame = (int)(time.tv_nsec + time.tv_sec * 1000000000);

            spawn_food(fld);
            clear_field(fld);
            compose(snk, fld);
            draw(fld);
            game_status = action(snk, fld);
        }
    }
    system("clear");
    printf("\nyou lose\n");
    set_old_terminal_mode();
    return 0;
}