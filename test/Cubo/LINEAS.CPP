#include <dos.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <conio.h>
#include <graphics.h>

// Posicion de cada vértice del cubo
float cube[8][3] = {
    {-100,-100,-100},{100,-100,-100},{100,100,-100},{-100,100,-100},
    {-100,-100,100},{100,-100,100},{100,100,100},{-100,100,100}
};

// Aristas del cubo (pares de vértices conectados)
int edges[12][2] = {
    {0,1},{1,2},{2,3},{3,0},
    {4,5},{5,6},{6,7},{7,4},
    {0,4},{1,5},{2,6},{3,7}
};

// Rotación en plano (usado para rotar en X o Y)
void rotate(float *a, float *b, float angle) {
    float temp = *a;
    *a = *a * cos(angle) - *b * sin(angle);
    *b = temp * sin(angle) + *b * cos(angle);
}

// Proyección 3D a 2D (simplificada)
void project(float x, float y, float z, int *sx, int *sy) {
    float dz = z + 300.0; // Asegura que nunca sea < 1
    if (dz < 1.0) dz = 1.0;
    float scale = 100.0 / dz;

    *sx = 320 + (int)(x * scale);
    *sy = 240 - (int)(y * scale);
}

int main() {
    int gd = DETECT, gm;
    clrscr();
    initgraph(&gd, &gm, "BGI");

    float angle = 0;
    int i, frame;
    int projected[8][2];

    for (frame = 0; frame < 500; frame++) {
        // Rotar el cubo
        for (i = 0; i < 8; i++) {
            float x = cube[i][0];
            float y = cube[i][1];
            float z = cube[i][2];
            // Rotar en eje Y y X
            rotate(&z, &x, angle * 1);   // Eje Z
            rotate(&y, &z, angle * 1);   // Eje X
            rotate(&y, &x, angle * 1); // Eje Y
            project(x, y, z, &projected[i][0], &projected[i][1]);
        }

        // Dibujar aristas
        setcolor(WHITE);
        for (i = 0; i < 12; i++) { //12 aristas
            int a = edges[i][0];
            int b = edges[i][1];
            line(projected[a][0], projected[a][1],
                 projected[b][0], projected[b][1]);
        }

        delay(34);
        angle += 0.05;
        //angle += 1;
        cleardevice();
    }

    setcolor(RED);
    outtextxy(10, 10, "Fin del giro. Presiona una tecla...");
    //getch();
    closegraph();
    return 0;
}