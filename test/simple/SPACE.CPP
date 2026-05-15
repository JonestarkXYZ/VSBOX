#include <graphics.h>
#include <conio.h>
#include <stdlib.h>
#include <time.h>
#include <dos.h>

#define MAX_POINTS 20
#define SCREEN_W 640
#define SCREEN_H 480

// Estructura para un punto 2D
typedef struct {
    int x;
    int y;
} Point;

Point leftTerrain[MAX_POINTS];
Point rightTerrain[MAX_POINTS];

// Inicializa puntos del terreno
void initTerrain() {
    int i;
    for (i = 0; i < MAX_POINTS; i++) {
        leftTerrain[i].x = 100 + rand() % 30;       // Lado izquierdo montañoso
        leftTerrain[i].y = i * (SCREEN_H / MAX_POINTS);

        rightTerrain[i].x = SCREEN_W - 100 - rand() % 30; // Lado derecho
        rightTerrain[i].y = leftTerrain[i].y;
    }
}

// Actualiza terreno (baja puntos y genera nuevos)
void updateTerrain() {
    int i;
    for (i = MAX_POINTS - 1; i > 0; i--) {
        leftTerrain[i] = leftTerrain[i - 1];
        rightTerrain[i] = rightTerrain[i - 1];
    }

    // Nueva "montaña" arriba
    leftTerrain[0].x = 100 + rand() % 30;
    leftTerrain[0].y = 0;

    rightTerrain[0].x = SCREEN_W - 100 - rand() % 30;
    rightTerrain[0].y = 0;

    // Bajar todos los puntos
    for (i = 0; i < MAX_POINTS; i++) {
        leftTerrain[i].y += 24;  // Avanza en Y
        rightTerrain[i].y += 24;
    }
}

// Dibuja las líneas del terreno
void drawTerrain() {
    int i;
    setcolor(WHITE);

    for (i = 0; i < MAX_POINTS - 1; i++) {
        line(leftTerrain[i].x, leftTerrain[i].y,
             leftTerrain[i + 1].x, leftTerrain[i + 1].y);

        line(rightTerrain[i].x, rightTerrain[i].y,
             rightTerrain[i + 1].x, rightTerrain[i + 1].y);
    }
}

int main() {
    int gd = DETECT, gm;
    initgraph(&gd, &gm, "BGI");
    srand(time(NULL));  // Para terreno aleatorio

    initTerrain();

    while (!kbhit()) {
        cleardevice();

        drawTerrain();
        updateTerrain();

        delay(33);  // ~30 FPS  
    }

    closegraph();
    return 0;
}
