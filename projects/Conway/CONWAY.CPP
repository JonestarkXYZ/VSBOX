#include <graphics.h>
#include <conio.h>
#include <dos.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>

/* Tamano de pantalla esperado por el driver BGI VGA usado en DOSBox. */
#define SCREEN_W 640
#define SCREEN_H 480

/* Cada celda mide 8x8 pixeles: 80 columnas y 54 filas visibles. */
#define CELL_SIZE 8
#define GRID_COLS 80
#define GRID_ROWS 54
#define GRID_W (GRID_COLS * CELL_SIZE)
#define GRID_H (GRID_ROWS * CELL_SIZE)

/* La pantalla se revisa con frecuencia, pero la simulacion avanza por temporizador. */
#define FRAME_DELAY 5
#define MIN_STEP_DELAY 1
#define MAX_STEP_DELAY 1000

/* Codigos de teclado extendido usados por las flechas en Turbo C. */
#define KEY_EXTENDED_1 0
#define KEY_EXTENDED_2 224
#define KEY_UP 72
#define KEY_DOWN 80

/* Ratios INT 33h funcion 0F: valores mayores hacen mas lento el puntero. */
#define MOUSE_SPEED_X 16
#define MOUSE_SPEED_Y 32

/* Botones reportados por el driver de mouse DOS INT 33h. */
#define MOUSE_LEFT 1
#define MOUSE_RIGHT 2

/* La grilla actual y la siguiente se mantienen globales para no usar stack. */
unsigned char world[GRID_ROWS][GRID_COLS];
unsigned char nextWorld[GRID_ROWS][GRID_COLS];

/* Estado global del mouse DOS. */
union REGS inRegs, outRegs;
int mouseReady = 0;
int mouseX = 0;
int mouseY = 0;
int mouseButtons = 0;

/* Prototipo necesario porque stepWorld puede dibujar celdas que cambiaron. */
void drawCell(int col, int row, int alive);
void drawHud(int running, long generation, int speedDelay);

/* Limpia todas las celdas del tablero. */
void clearWorld()
{
    int row, col;

    for (row = 0; row < GRID_ROWS; row++) {
        for (col = 0; col < GRID_COLS; col++) {
            world[row][col] = 0;
        }
    }
}

/* Llena el tablero con una densidad moderada de celulas vivas. */
void randomWorld()
{
    int row, col;

    for (row = 0; row < GRID_ROWS; row++) {
        for (col = 0; col < GRID_COLS; col++) {
            world[row][col] = (rand() % 5 == 0) ? 1 : 0;
        }
    }
}

/* Cuenta las celulas vivas que rodean una posicion. Los bordes no envuelven. */
int countNeighbors(int row, int col)
{
    int dr, dc;
    int nr, nc;
    int count = 0;

    for (dr = -1; dr <= 1; dr++) {
        for (dc = -1; dc <= 1; dc++) {
            if (dr == 0 && dc == 0) {
                continue;
            }

            nr = row + dr;
            nc = col + dc;

            if (nr >= 0 && nr < GRID_ROWS && nc >= 0 && nc < GRID_COLS) {
                if (world[nr][nc]) {
                    count++;
                }
            }
        }
    }

    return count;
}

/* Aplica una generacion y redibuja solo las celdas que cambiaron. */
int stepWorld()
{
    int row, col;
    int neighbors;
    int changed = 0;

    for (row = 0; row < GRID_ROWS; row++) {
        for (col = 0; col < GRID_COLS; col++) {
            neighbors = countNeighbors(row, col);

            /* Celula viva: sobrevive con 2 o 3 vecinas; si no, muere. */
            if (world[row][col]) {
                nextWorld[row][col] = (neighbors == 2 || neighbors == 3) ? 1 : 0;
            }

            /* Celula muerta: nace exactamente con 3 vecinas vivas. */
            else {
                nextWorld[row][col] = (neighbors == 3) ? 1 : 0;
            }
        }
    }

    for (row = 0; row < GRID_ROWS; row++) {
        for (col = 0; col < GRID_COLS; col++) {
            if (world[row][col] != nextWorld[row][col]) {
                world[row][col] = nextWorld[row][col];
                drawCell(col, row, world[row][col]);
                changed++;
            }
        }
    }

    return changed;
}

/* Cuenta celulas vivas para mostrar informacion util en pantalla. */
int countAlive()
{
    int row, col;
    int total = 0;

    for (row = 0; row < GRID_ROWS; row++) {
        for (col = 0; col < GRID_COLS; col++) {
            if (world[row][col]) {
                total++;
            }
        }
    }

    return total;
}

/* Inicializa el mouse mediante INT 33h y fija sus limites al modo grafico. */
void initMouse()
{
    inRegs.x.ax = 0;
    int86(0x33, &inRegs, &outRegs);
    mouseReady = outRegs.x.ax;

    if (!mouseReady) {
        return;
    }

    /* Rango horizontal de 0 a 639 pixeles. */
    inRegs.x.ax = 7;
    inRegs.x.cx = 0;
    inRegs.x.dx = SCREEN_W - 1;
    int86(0x33, &inRegs, &outRegs);

    /* Rango vertical de 0 a 479 pixeles. */
    inRegs.x.ax = 8;
    inRegs.x.cx = 0;
    inRegs.x.dx = SCREEN_H - 1;
    int86(0x33, &inRegs, &outRegs);

    /* Reduce la velocidad del mouse en DOSBox usando mickeys por 8 pixeles. */
    inRegs.x.ax = 15;
    inRegs.x.cx = MOUSE_SPEED_X;
    inRegs.x.dx = MOUSE_SPEED_Y;
    int86(0x33, &inRegs, &outRegs);

    /* Muestra el cursor nativo del driver de mouse DOS. */
    inRegs.x.ax = 1;
    int86(0x33, &inRegs, &outRegs);
}

/* Oculta el cursor durante el redibujado para reducir rastros visuales. */
void hideMouse()
{
    if (mouseReady) {
        inRegs.x.ax = 2;
        int86(0x33, &inRegs, &outRegs);
    }
}

/* Vuelve a mostrar el cursor despues de dibujar la escena. */
void showMouse()
{
    if (mouseReady) {
        inRegs.x.ax = 1;
        int86(0x33, &inRegs, &outRegs);
    }
}

/* Lee posicion y botones del mouse. */
void readMouse()
{
    if (!mouseReady) {
        mouseButtons = 0;
        return;
    }

    inRegs.x.ax = 3;
    int86(0x33, &inRegs, &outRegs);

    mouseButtons = outRegs.x.bx;
    mouseX = outRegs.x.cx;
    mouseY = outRegs.x.dx;
}

/* Dibuja o borra el interior de una celda sin tocar las lineas de grilla. */
void drawCell(int col, int row, int alive)
{
    int x1, y1;
    int x2, y2;
    int color;

    x1 = col * CELL_SIZE + 1;
    y1 = row * CELL_SIZE + 1;
    x2 = (col + 1) * CELL_SIZE - 2;
    y2 = (row + 1) * CELL_SIZE - 2;
    color = alive ? LIGHTGREEN : BLACK;

    setfillstyle(SOLID_FILL, color);
    bar(x1, y1, x2, y2);
}

/* Usa el mouse para crear o borrar celulas dentro de la grilla. */
int handleMousePaint()
{
    int col, row;
    unsigned char newValue;

    if (!mouseReady) {
        return 0;
    }

    if (mouseX < 0 || mouseX >= GRID_W || mouseY < 0 || mouseY >= GRID_H) {
        return 0;
    }

    col = mouseX / CELL_SIZE;
    row = mouseY / CELL_SIZE;
    newValue = world[row][col];

    /* Boton izquierdo crea celulas; boton derecho las borra. */
    if (mouseButtons & MOUSE_LEFT) {
        newValue = 1;
    }
    else if (mouseButtons & MOUSE_RIGHT) {
        newValue = 0;
    }
    else {
        return 0;
    }

    if (world[row][col] == newValue) {
        return 0;
    }

    world[row][col] = newValue;
    hideMouse();
    drawCell(col, row, world[row][col]);
    showMouse();
    return 1;
}

/* Dibuja la rejilla y todas las celulas vivas. */
void drawWorld()
{
    int row, col;
    int x, y;

    setcolor(DARKGRAY);
    for (x = 0; x < GRID_W; x += CELL_SIZE) {
        line(x, 0, x, GRID_H - 1);
    }
    line(GRID_W - 1, 0, GRID_W - 1, GRID_H - 1);

    for (y = 0; y <= GRID_H; y += CELL_SIZE) {
        line(0, y, GRID_W - 1, y);
    }

    setfillstyle(SOLID_FILL, LIGHTGREEN);
    setcolor(LIGHTGREEN);

    for (row = 0; row < GRID_ROWS; row++) {
        for (col = 0; col < GRID_COLS; col++) {
            if (world[row][col]) {
                drawCell(col, row, 1);
            }
        }
    }
}

/* Redibujo completo usado solo al iniciar, limpiar o generar un tablero nuevo. */
void drawFullFrame(int running, long generation, int speedDelay)
{
    cleardevice();
    drawWorld();
    drawHud(running, generation, speedDelay);
}

/* Dibuja la barra inferior con estado y controles disponibles. */
void drawHud(int running, long generation, int speedDelay)
{
    char info[120];

    setfillstyle(SOLID_FILL, BLACK);
    bar(0, GRID_H + 1, SCREEN_W - 1, SCREEN_H - 1);

    setcolor(WHITE);
    line(0, GRID_H, SCREEN_W - 1, GRID_H);

    setcolor(YELLOW);
    outtextxy(8, GRID_H + 4, "Juego de la Vida de Conway");

    sprintf(info, "Estado:%s  Gen:%ld  Vivas:%d  Delay:%dms  Mouse:%s",
            running ? "RUN" : "PAUSA",
            generation,
            countAlive(),
            speedDelay,
            mouseReady ? "OK" : "NO");
    setcolor(LIGHTCYAN);
    outtextxy(8, GRID_H + 16, info);

    setcolor(LIGHTGRAY);
    outtextxy(8, GRID_H + 28,
              "P/ESP play-pausa | N paso | R aleatorio | C limpiar | Flechas velocidad");
    outtextxy(8, GRID_H + 40,
              "ESC salir | Mouse: boton izquierdo crea, boton derecho borra");
}

/* Procesa teclado sin bloquear la simulacion. */
void handleKeyboard(int *running, int *exitGame, int *speedDelay,
                    long *generation, int *fullRedrawNeeded, int *hudDirty,
                    int *stepRequested, int *elapsedMs)
{
    int key;

    while (kbhit()) {
        key = getch();

        /* Las flechas llegan como tecla extendida: prefijo 0 o 224 + codigo. */
        if (key == KEY_EXTENDED_1 || key == KEY_EXTENDED_2) {
            key = getch();

            if (key == KEY_UP) {
                if (*speedDelay <= MIN_STEP_DELAY + 10) {
                    *speedDelay = MIN_STEP_DELAY;
                }
                else {
                    *speedDelay -= 10;
                }
                *elapsedMs = 0;
                *hudDirty = 1;
            }
            else if (key == KEY_DOWN) {
                if (*speedDelay >= MAX_STEP_DELAY - 10) {
                    *speedDelay = MAX_STEP_DELAY;
                }
                else {
                    *speedDelay += 10;
                }
                *elapsedMs = 0;
                *hudDirty = 1;
            }
            continue;
        }

        if (key == 27) {
            *exitGame = 1;
        }
        else if (key == ' ' || key == 'p' || key == 'P') {
            *running = !(*running);
            *elapsedMs = 0;
            *hudDirty = 1;
        }
        else if (key == 'c' || key == 'C') {
            clearWorld();
            *generation = 0;
            *elapsedMs = 0;
            *fullRedrawNeeded = 1;
            *hudDirty = 1;
        }
        else if (key == 'r' || key == 'R') {
            randomWorld();
            *generation = 0;
            *elapsedMs = 0;
            *fullRedrawNeeded = 1;
            *hudDirty = 1;
        }
        else if (key == 'n' || key == 'N') {
            *stepRequested = 1;
            *elapsedMs = 0;
        }
        else if (key == '+' || key == '=') {
            if (*speedDelay <= MIN_STEP_DELAY + 10) {
                *speedDelay = MIN_STEP_DELAY;
            }
            else {
                *speedDelay -= 10;
            }
            *elapsedMs = 0;
            *hudDirty = 1;
        }
        else if (key == '-' || key == '_') {
            if (*speedDelay >= MAX_STEP_DELAY - 10) {
                *speedDelay = MAX_STEP_DELAY;
            }
            else {
                *speedDelay += 10;
            }
            *elapsedMs = 0;
            *hudDirty = 1;
        }
    }
}

int main()
{
    int gd = DETECT, gm;
    int running = 0;
    int exitGame = 0;
    int speedDelay = 120;
    int elapsedMs = 0;
    int fullRedrawNeeded = 1;
    int hudDirty = 1;
    int stepRequested = 0;
    long generation = 0;

    clrscr();
    initgraph(&gd, &gm, "BGI");
    srand((unsigned)time(NULL));

    clearWorld();
    randomWorld();
    initMouse();

    while (!exitGame) {
        readMouse();
        handleKeyboard(&running, &exitGame, &speedDelay, &generation,
                       &fullRedrawNeeded, &hudDirty, &stepRequested, &elapsedMs);

        if (running) {
            elapsedMs += FRAME_DELAY;
            if (elapsedMs >= speedDelay) {
                stepRequested = 1;
                elapsedMs = 0;
            }
        }

        if (fullRedrawNeeded) {
            hideMouse();
            drawFullFrame(running, generation, speedDelay);
            showMouse();
            fullRedrawNeeded = 0;
            hudDirty = 0;
        }
        else {
            if (mouseButtons & (MOUSE_LEFT | MOUSE_RIGHT)) {
                handleMousePaint();
            }

            if (stepRequested) {
                hideMouse();
                stepWorld();
                showMouse();
                generation++;
                stepRequested = 0;
            }

            if (hudDirty) {
                hideMouse();
                drawHud(running, generation, speedDelay);
                showMouse();
                hudDirty = 0;
            }
        }

        delay(FRAME_DELAY);
    }

    hideMouse();
    closegraph();
    return 0;
}
