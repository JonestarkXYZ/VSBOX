#include <graphics.h>
#include <conio.h>
#include <dos.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>

/* Snake clasico 2D para Turbo C/C++ y DOSBox.
   Controles: flechas o WASD. ESC sale, P/espacio pausa y R reinicia. */

#define SCREEN_W 640
#define SCREEN_H 480

#define CELL_SIZE 16
#define BOARD_COLS 30
#define BOARD_ROWS 22
#define BOARD_X 80
#define BOARD_Y 78
#define BOARD_W (BOARD_COLS * CELL_SIZE)
#define BOARD_H (BOARD_ROWS * CELL_SIZE)

#define MAX_SNAKE (BOARD_COLS * BOARD_ROWS)
#define START_LENGTH 5

#define SPEED_MIN_LEVEL 1
#define SPEED_START_LEVEL 1
#define SPEED_MAX_LEVEL 50
#define SPEED_PER_FOOD 1
#define SPEED_SLOWEST_DELAY_MS 116
#define SPEED_DELAY_STEP_MS 4

#define KEY_ESC 27
#define KEY_SPACE 32
#define KEY_ENTER 13
#define KEY_EXTENDED_1 0
#define KEY_EXTENDED_2 224
#define KEY_UP 72
#define KEY_DOWN 80
#define KEY_LEFT 75
#define KEY_RIGHT 77

#define STATE_PLAYING 0
#define STATE_PAUSED 1
#define STATE_GAME_OVER 2
#define STATE_WIN 3

#define MODE_CLASSIC 0
#define MODE_INFINITE 1

int snakeX[MAX_SNAKE];
int snakeY[MAX_SNAKE];
int snakeLength;

int foodX;
int foodY;
int score;
int gameState;
int gameMode;
int speedLevel;
int delayMs;

int dirX;
int dirY;
int pendingDirX;
int pendingDirY;
int inputLocked;
int quitGame;

int oldHeadX;
int oldHeadY;
int oldTailX;
int oldTailY;
int movedThisFrame;
int erasedTailThisFrame;
int foodChangedThisFrame;
int hudDirty;
int fullRedrawNeeded;
int renderedState = -1;

void updateDelayFromSpeed() {
    /* En delay(), menos milisegundos significa mayor velocidad real. */
    delayMs = SPEED_SLOWEST_DELAY_MS -
              (speedLevel - SPEED_MIN_LEVEL) * SPEED_DELAY_STEP_MS;
}

void increaseSpeedAfterFood() {
    /* Cada fruta sube la velocidad hasta el limite maximo configurado. */
    speedLevel += SPEED_PER_FOOD;
    if (speedLevel > SPEED_MAX_LEVEL) {
        speedLevel = SPEED_MAX_LEVEL;
    }

    updateDelayFromSpeed();
}

void initVideo() {
    int gd;
    int gm;

    /* Usa el driver autodetectado de BGI. Ejecutado desde C:\TURBOC3,
       la ruta relativa "BGI" funciona con las tareas del repositorio. */
    gd = DETECT;
    initgraph(&gd, &gm, "BGI");
}

void shutdownVideo() {
    closegraph();
}

int isSnakeCell(int x, int y) {
    int i;

    /* Revisa si una celda pertenece al cuerpo actual de la serpiente. */
    for (i = 0; i < snakeLength; i++) {
        if (snakeX[i] == x && snakeY[i] == y) {
            return 1;
        }
    }

    return 0;
}

void placeFood() {
    int attempts;
    int x;
    int y;

    /* Intenta colocar comida aleatoria fuera del cuerpo. */
    for (attempts = 0; attempts < 400; attempts++) {
        x = rand() % BOARD_COLS;
        y = rand() % BOARD_ROWS;

        if (!isSnakeCell(x, y)) {
            foodX = x;
            foodY = y;
            return;
        }
    }

    /* Respaldo determinista si el tablero esta casi lleno. */
    for (y = 0; y < BOARD_ROWS; y++) {
        for (x = 0; x < BOARD_COLS; x++) {
            if (!isSnakeCell(x, y)) {
                foodX = x;
                foodY = y;
                return;
            }
        }
    }
}

void resetGame() {
    int i;
    int startX;
    int startY;

    /* Estado inicial centrado, moviendose hacia la derecha. */
    snakeLength = START_LENGTH;
    startX = BOARD_COLS / 2;
    startY = BOARD_ROWS / 2;

    for (i = 0; i < snakeLength; i++) {
        snakeX[i] = startX - i;
        snakeY[i] = startY;
    }

    dirX = 1;
    dirY = 0;
    pendingDirX = dirX;
    pendingDirY = dirY;
    inputLocked = 0;

    score = 0;
    speedLevel = SPEED_START_LEVEL;
    updateDelayFromSpeed();
    gameState = STATE_PLAYING;
    movedThisFrame = 0;
    erasedTailThisFrame = 0;
    foodChangedThisFrame = 0;
    hudDirty = 1;
    fullRedrawNeeded = 1;

    placeFood();
}

int hitsBody(int x, int y, int willGrow) {
    int i;
    int limit;

    /* Si no crece, se permite entrar en la antigua cola porque esta avanza. */
    limit = snakeLength;
    if (!willGrow) {
        limit = snakeLength - 1;
    }

    for (i = 0; i < limit; i++) {
        if (snakeX[i] == x && snakeY[i] == y) {
            return 1;
        }
    }

    return 0;
}

void updateSnake() {
    int nextX;
    int nextY;
    int willGrow;
    int i;

    movedThisFrame = 0;
    erasedTailThisFrame = 0;
    foodChangedThisFrame = 0;

    if (gameState != STATE_PLAYING) {
        return;
    }

    /* Aplica la direccion aceptada durante el ultimo intervalo. */
    dirX = pendingDirX;
    dirY = pendingDirY;

    oldHeadX = snakeX[0];
    oldHeadY = snakeY[0];
    oldTailX = snakeX[snakeLength - 1];
    oldTailY = snakeY[snakeLength - 1];

    nextX = snakeX[0] + dirX;
    nextY = snakeY[0] + dirY;

    /* En modo infinito, salir por un borde reaparece por el borde opuesto. */
    if (gameMode == MODE_INFINITE) {
        if (nextX < 0) {
            nextX = BOARD_COLS - 1;
        } else if (nextX >= BOARD_COLS) {
            nextX = 0;
        }

        if (nextY < 0) {
            nextY = BOARD_ROWS - 1;
        } else if (nextY >= BOARD_ROWS) {
            nextY = 0;
        }
    } else if (nextX < 0 || nextX >= BOARD_COLS || nextY < 0 || nextY >= BOARD_ROWS) {
        gameState = STATE_GAME_OVER;
        return;
    }

    willGrow = (nextX == foodX && nextY == foodY);

    if (hitsBody(nextX, nextY, willGrow)) {
        gameState = STATE_GAME_OVER;
        return;
    }

    if (willGrow && snakeLength < MAX_SNAKE) {
        snakeLength++;
    } else {
        erasedTailThisFrame = 1;
    }

    /* Desplaza el cuerpo desde la cola hacia la cabeza. */
    for (i = snakeLength - 1; i > 0; i--) {
        snakeX[i] = snakeX[i - 1];
        snakeY[i] = snakeY[i - 1];
    }

    snakeX[0] = nextX;
    snakeY[0] = nextY;
    movedThisFrame = 1;

    if (willGrow) {
        score += 10;
        hudDirty = 1;

        increaseSpeedAfterFood();

        if (snakeLength >= MAX_SNAKE) {
            gameState = STATE_WIN;
        } else {
            placeFood();
            foodChangedThisFrame = 1;
        }
    }

    inputLocked = 0;
}

void requestDirection(int newDirX, int newDirY) {
    if (gameState != STATE_PLAYING || inputLocked) {
        return;
    }

    /* Evita reversa instantanea contra el propio cuerpo. */
    if (newDirX == -dirX && newDirY == -dirY) {
        return;
    }

    pendingDirX = newDirX;
    pendingDirY = newDirY;
    inputLocked = 1;
}

void handleInput() {
    int key;

    while (kbhit()) {
        key = getch();

        /* Las flechas llegan como tecla extendida: prefijo 0 o 224 + codigo. */
        if (key == KEY_EXTENDED_1 || key == KEY_EXTENDED_2) {
            key = getch();

            if (key == KEY_UP) {
                requestDirection(0, -1);
            } else if (key == KEY_DOWN) {
                requestDirection(0, 1);
            } else if (key == KEY_LEFT) {
                requestDirection(-1, 0);
            } else if (key == KEY_RIGHT) {
                requestDirection(1, 0);
            }
        } else {
            if (key == KEY_ESC) {
                quitGame = 1;
            } else if (key == 'w' || key == 'W') {
                requestDirection(0, -1);
            } else if (key == 's' || key == 'S') {
                requestDirection(0, 1);
            } else if (key == 'a' || key == 'A') {
                requestDirection(-1, 0);
            } else if (key == 'd' || key == 'D') {
                requestDirection(1, 0);
            } else if (key == 'p' || key == 'P' || key == KEY_SPACE) {
                if (gameState == STATE_PLAYING) {
                    gameState = STATE_PAUSED;
                    inputLocked = 0;
                } else if (gameState == STATE_PAUSED) {
                    gameState = STATE_PLAYING;
                    inputLocked = 0;
                    fullRedrawNeeded = 1;
                }
            } else if (key == 'r' || key == 'R') {
                resetGame();
            }
        }
    }
}

void drawCell(int col, int row, int color) {
    int x1;
    int y1;
    int x2;
    int y2;

    /* Dibuja dentro de la celda, sin tocar las lineas de la grilla. */
    x1 = BOARD_X + col * CELL_SIZE + 1;
    y1 = BOARD_Y + row * CELL_SIZE + 1;
    x2 = BOARD_X + (col + 1) * CELL_SIZE - 2;
    y2 = BOARD_Y + (row + 1) * CELL_SIZE - 2;

    setfillstyle(SOLID_FILL, color);
    bar(x1, y1, x2, y2);
}

void clearCell(int col, int row) {
    /* Borra solo el interior de una celda; las lineas de grilla quedan intactas. */
    drawCell(col, row, BLACK);
}

void drawBoard() {
    int x;
    int y;

    /* Fondo del area de juego y borde principal. */
    setfillstyle(SOLID_FILL, BLACK);
    bar(BOARD_X - 2, BOARD_Y - 2, BOARD_X + BOARD_W + 1, BOARD_Y + BOARD_H + 1);
    setcolor(DARKGRAY);
    rectangle(BOARD_X - 1, BOARD_Y - 1, BOARD_X + BOARD_W, BOARD_Y + BOARD_H);

    /* Grilla tenue para hacer legible el movimiento por celdas. */
    setcolor(DARKGRAY);
    for (x = 1; x < BOARD_COLS; x++) {
        line(BOARD_X + x * CELL_SIZE, BOARD_Y, BOARD_X + x * CELL_SIZE, BOARD_Y + BOARD_H);
    }

    for (y = 1; y < BOARD_ROWS; y++) {
        line(BOARD_X, BOARD_Y + y * CELL_SIZE, BOARD_X + BOARD_W, BOARD_Y + y * CELL_SIZE);
    }
}

void drawHudStats() {
    char text[80];

    /* Limpia solo el area variable del HUD para no repintar toda la pantalla. */
    setfillstyle(SOLID_FILL, BLACK);
    bar(BOARD_X, 34, BOARD_X + 285, 48);

    setcolor(WHITE);
    settextstyle(DEFAULT_FONT, HORIZ_DIR, 1);
    sprintf(text, "Puntos: %d   Largo: %d   Vel: %d/%d",
            score, snakeLength, speedLevel, SPEED_MAX_LEVEL);
    outtextxy(BOARD_X, 36, text);
}

void drawHud() {
    setcolor(WHITE);
    settextstyle(DEFAULT_FONT, HORIZ_DIR, 1);
    if (gameMode == MODE_INFINITE) {
        outtextxy(BOARD_X, 18, "SNAKE 2D - MODO INFINITO");
    } else {
        outtextxy(BOARD_X, 18, "SNAKE 2D - MODO CLASICO");
    }

    drawHudStats();
    outtextxy(BOARD_X, 56, "Flechas/WASD mover | P pausa | R reinicia | ESC sale");
}

void drawSnake() {
    int i;

    /* Cabeza en verde claro, cuerpo en verde para distinguir direccion. */
    drawCell(snakeX[0], snakeY[0], LIGHTGREEN);

    for (i = 1; i < snakeLength; i++) {
        drawCell(snakeX[i], snakeY[i], GREEN);
    }
}

void drawFoodPixel(int baseX, int baseY, int px, int py, int color) {
    int x;
    int y;

    /* Cada bloque funciona como un pixel grande de 2x2 dentro de la celda. */
    x = baseX + px * 2;
    y = baseY + py * 2;

    setfillstyle(SOLID_FILL, color);
    bar(x, y, x + 1, y + 1);
}

void drawFood() {
    int baseX;
    int baseY;
    int x;

    /* Manzana pixel art de 14x14 px, centrada sin tocar la grilla. */
    clearCell(foodX, foodY);
    baseX = BOARD_X + foodX * CELL_SIZE + 1;
    baseY = BOARD_Y + foodY * CELL_SIZE + 1;

    drawFoodPixel(baseX, baseY, 3, 0, BROWN);
    drawFoodPixel(baseX, baseY, 3, 1, BROWN);
    drawFoodPixel(baseX, baseY, 4, 1, GREEN);
    drawFoodPixel(baseX, baseY, 5, 1, LIGHTGREEN);

    for (x = 2; x <= 4; x++) {
        drawFoodPixel(baseX, baseY, x, 2, RED);
    }

    for (x = 1; x <= 5; x++) {
        drawFoodPixel(baseX, baseY, x, 3, LIGHTRED);
    }

    for (x = 0; x <= 6; x++) {
        drawFoodPixel(baseX, baseY, x, 4, LIGHTRED);
    }

    for (x = 1; x <= 5; x++) {
        drawFoodPixel(baseX, baseY, x, 5, RED);
    }

    for (x = 2; x <= 4; x++) {
        drawFoodPixel(baseX, baseY, x, 6, RED);
    }

    drawFoodPixel(baseX, baseY, 2, 3, WHITE);
}

void drawCenteredMessage(char *line1, char *line2) {
    int boxX1;
    int boxY1;
    int boxX2;
    int boxY2;

    boxX1 = 160;
    boxY1 = 198;
    boxX2 = 480;
    boxY2 = 282;

    /* Caja de estado para pausa, derrota o victoria. */
    setfillstyle(SOLID_FILL, BLUE);
    bar(boxX1, boxY1, boxX2, boxY2);
    setcolor(WHITE);
    rectangle(boxX1, boxY1, boxX2, boxY2);

    settextstyle(DEFAULT_FONT, HORIZ_DIR, 2);
    outtextxy(boxX1 + 44, boxY1 + 18, line1);

    settextstyle(DEFAULT_FONT, HORIZ_DIR, 1);
    outtextxy(boxX1 + 36, boxY1 + 58, line2);
}

void drawStateOverlay() {
    /* Solo se dibuja cuando cambia el estado; asi no parpadea el mensaje. */
    if (gameState == STATE_PAUSED) {
        drawCenteredMessage("PAUSA", "P/Espacio continua  R reinicia");
    } else if (gameState == STATE_GAME_OVER) {
        drawCenteredMessage("GAME OVER", "R reinicia  ESC sale");
    } else if (gameState == STATE_WIN) {
        drawCenteredMessage("GANASTE", "R reinicia  ESC sale");
    }
}

void drawFullGame() {
    /* Redibujo completo reservado para inicio, reinicio o salida de pausa. */
    cleardevice();

    drawHud();
    drawBoard();
    drawFood();
    drawSnake();
    drawStateOverlay();

    hudDirty = 0;
    movedThisFrame = 0;
    erasedTailThisFrame = 0;
    foodChangedThisFrame = 0;
    fullRedrawNeeded = 0;
    renderedState = gameState;
}

void drawIncrementalFrame() {
    if (movedThisFrame) {
        if (erasedTailThisFrame) {
            clearCell(oldTailX, oldTailY);
        }

        /* La cabeza anterior pasa a ser cuerpo y la nueva cabeza queda resaltada. */
        drawCell(oldHeadX, oldHeadY, GREEN);

        if (foodChangedThisFrame) {
            drawFood();
        }

        drawCell(snakeX[0], snakeY[0], LIGHTGREEN);
    }

    if (hudDirty) {
        drawHudStats();
        hudDirty = 0;
    }

    movedThisFrame = 0;
    erasedTailThisFrame = 0;
    foodChangedThisFrame = 0;
}

void renderFrame() {
    if (fullRedrawNeeded) {
        drawFullGame();
        return;
    }

    drawIncrementalFrame();

    if (gameState != renderedState) {
        if (gameState == STATE_PLAYING) {
            fullRedrawNeeded = 1;
            drawFullGame();
        } else {
            drawStateOverlay();
            renderedState = gameState;
        }
    }
}

void drawMenuOption(int y, char *title, char *detail, int selected) {
    int boxX1;
    int boxX2;

    boxX1 = 125;
    boxX2 = 515;

    /* Resalta la opcion activa sin usar animaciones para evitar parpadeo. */
    if (selected) {
        setfillstyle(SOLID_FILL, BLUE);
        setcolor(WHITE);
    } else {
        setfillstyle(SOLID_FILL, BLACK);
        setcolor(DARKGRAY);
    }

    bar(boxX1, y, boxX2, y + 74);
    rectangle(boxX1, y, boxX2, y + 74);

    setcolor(WHITE);
    settextstyle(DEFAULT_FONT, HORIZ_DIR, 2);
    outtextxy(boxX1 + 22, y + 12, title);

    settextstyle(DEFAULT_FONT, HORIZ_DIR, 1);
    outtextxy(boxX1 + 22, y + 48, detail);
}

void drawModeMenu(int selectedMode) {
    cleardevice();

    /* Pantalla inicial para elegir la regla de paredes antes de jugar. */
    setcolor(WHITE);
    settextstyle(DEFAULT_FONT, HORIZ_DIR, 3);
    outtextxy(216, 56, "SNAKE");

    settextstyle(DEFAULT_FONT, HORIZ_DIR, 1);
    outtextxy(156, 112, "Selecciona el modo antes de iniciar la partida");

    drawMenuOption(160, "1  CLASICO", "Muere al chocar contra pared o cuerpo",
                   selectedMode == MODE_CLASSIC);
    drawMenuOption(254, "2  INFINITO", "La pared lleva al lado contrario",
                   selectedMode == MODE_INFINITE);

    setcolor(LIGHTGRAY);
    outtextxy(142, 370, "Flechas/WASD cambian | Enter/Espacio inicia | ESC sale");
}

int chooseGameMode() {
    int selectedMode;
    int key;
    int redrawMenu;

    selectedMode = MODE_CLASSIC;
    redrawMenu = 1;

    while (1) {
        if (redrawMenu) {
            drawModeMenu(selectedMode);
            redrawMenu = 0;
        }

        if (kbhit()) {
            key = getch();

            if (key == KEY_EXTENDED_1 || key == KEY_EXTENDED_2) {
                key = getch();

                if (key == KEY_UP || key == KEY_DOWN) {
                    selectedMode = 1 - selectedMode;
                    redrawMenu = 1;
                }
            } else if (key == KEY_ESC) {
                return -1;
            } else if (key == KEY_ENTER || key == KEY_SPACE) {
                return selectedMode;
            } else if (key == '1' || key == 'c' || key == 'C') {
                return MODE_CLASSIC;
            } else if (key == '2' || key == 'i' || key == 'I') {
                return MODE_INFINITE;
            } else if (key == 'w' || key == 'W' || key == 's' || key == 'S' ||
                       key == 'a' || key == 'A' || key == 'd' || key == 'D') {
                selectedMode = 1 - selectedMode;
                redrawMenu = 1;
            }
        }

        delay(20);
    }
}

int main() {
    srand((unsigned)time(NULL));

    initVideo();
    gameMode = chooseGameMode();
    if (gameMode < 0) {
        shutdownVideo();
        return 0;
    }

    resetGame();
    quitGame = 0;

    while (!quitGame) {
        handleInput();
        updateSnake();
        renderFrame();
        delay(delayMs);
    }

    shutdownVideo();
    return 0;
}
