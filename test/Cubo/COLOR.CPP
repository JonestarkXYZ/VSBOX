#include <graphics.h>
#include <dos.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <conio.h>


int projected[8][2];
float transformed[8][3];
int i, frame;

float cube[8][3] = {
    {-100,-100,-100},{100,-100,-100},{100,100,-100},{-100,100,-100},
    {-100,-100,100},{100,-100,100},{100,100,100},{-100,100,100}
};

int faces[6][4] = {
    {0, 1, 2, 3}, // trasera
    {4, 5, 6, 7}, // frontal
    {0, 3, 7, 4}, // izquierda
    {1, 2, 6, 5}, // derecha
    {3, 2, 6, 7}, // superior
    {0, 1, 5, 4}  // inferior
};

int faceColors[6] = {
    BLUE, RED, GREEN, MAGENTA, CYAN, YELLOW
};

void rotate(float* a, float* b, float angle) {
    float temp = *a;
    *a = *a * cos(angle) - *b * sin(angle);
    *b = temp * sin(angle) + *b * cos(angle);
}

void project(float x, float y, float z, int* sx, int* sy) {
    float dz = z + 300.0;
    if (dz < 1.0) dz = 1.0;
    float scale = 100.0 / dz;
    *sx = 320 + (int)(x * scale);
    *sy = 240 - (int)(y * scale);
}

// Calcular producto escalar de dos vectores 3D
float dot(float a[3], float b[3]) {
    return a[0]*b[0] + a[1]*b[1] + a[2]*b[2];
}

// Calcular vector normal de una cara (cruz entre 2 lados)
void faceNormal(int face[4], float out[3]) {
    float *p0 = transformed[face[0]];
    float *p1 = transformed[face[1]];
    float *p2 = transformed[face[2]];

    float u[3] = { p1[0]-p0[0], p1[1]-p0[1], p1[2]-p0[2] };
    float v[3] = { p2[0]-p0[0], p2[1]-p0[1], p2[2]-p0[2] };

    out[0] = u[1]*v[2] - u[2]*v[1];
    out[1] = u[2]*v[0] - u[0]*v[2];
    out[2] = u[0]*v[1] - u[1]*v[0];
}

int main() {
    int gd = DETECT, gm;
    initgraph(&gd, &gm, "BGI");
    float angle = 0;
    int poly[8];

    for (frame = 0; frame < 1000; frame++) {
        cleardevice();

        // Proyectar y rotar vértices
        for (i = 0; i < 8; i++) {
            float x = cube[i][0];
            float y = cube[i][1];
            float z = cube[i][2];

            rotate(&z, &x, angle);
            rotate(&y, &z, angle);
            rotate(&y, &x, angle);

            transformed[i][0] = x;
            transformed[i][1] = y;
            transformed[i][2] = z;

            project(x, y, z, &projected[i][0], &projected[i][1]);
        }

        // Dibujar solo las caras que miran hacia la cámara
        float view[3] = { 1, 0, 0 }; // dirección de la cámara (eje Z negativo)

        for (i = 0; i < 6; i++) {
            float normal[3];
            faceNormal(faces[i], normal);

            if (dot(normal, view) < 0) {
                // cara visible
                for (int j = 0; j < 4; j++) {
                    int idx = faces[i][j];
                    poly[j * 2]     = projected[idx][0];
                    poly[j * 2 + 1] = projected[idx][1];
                }

                setfillstyle(SOLID_FILL, faceColors[i]);
                fillpoly(4, poly);

                setcolor(BLACK); // contorno para que se note
                drawpoly(4, poly);
            }
        }

        delay(11);
        angle += 0.05;
    }

    closegraph();
    return 0;
}
