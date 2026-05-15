#include <graphics.h>
#include <stdio.h>
#include <conio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#define MAX_VERTICES 100
#define MAX_ARISTAS 100

struct Vec3 {
    float x, y, z;
};

struct Vec3 vertices[MAX_VERTICES];
int aristas[MAX_ARISTAS][2];
int numVertices = 0, numAristas = 0;
char palabra[20];
float x, y, z;
int a, b;
int i;
float angle = 0.0;
int i, frame;
int px[MAX_VERTICES], py[MAX_VERTICES];

void rotate(float* a, float* b, float angle) {
    float cosA = cos(angle);
    float sinA = sin(angle);
    float temp = *a * cosA - *b * sinA;
    *b = *a * sinA + *b * cosA;
    *a = temp;
}

void project(float x, float y, float z, int* px, int* py) {
    float scale = 200.0 / (z + 300);  // Alejar para evitar división por 0
    *px = 320 + (int)(x * scale);
    *py = 240 - (int)(y * scale);
}

void loadModel(const char* path) {
    FILE* archivo = fopen(path, "r");
    if (!archivo) {
        printf("No se pudo abrir el modelo\n");
        getch();
        exit(1);
    }

    while (fscanf(archivo, "%s", palabra) == 1) {
        if (strcmp(palabra, "VERTICES") == 0) {
            fscanf(archivo, "%d", &numVertices);
            for (i = 0; i < numVertices; i++) {
                fscanf(archivo, "%f %f %f", &x, &y, &z);
                vertices[i].x = x;
                vertices[i].y = y;
                vertices[i].z = z;
            }
        } else if (strcmp(palabra, "ARISTAS") == 0) {
            fscanf(archivo, "%d", &numAristas);
            for (i = 0; i < numAristas; i++) {
                fscanf(archivo, "%d %d", &a, &b);
                aristas[i][0] = a;
                aristas[i][1] = b;
            }
        }
    }

    fclose(archivo);
}

void drawModel(int color) {
    for (frame = 0; frame < 200; frame++) {
        // Procesar rotación
        for (i = 0; i < numVertices; i++) {
            float x = vertices[i].x;
            float y = vertices[i].y;
            float z = vertices[i].z;

            // Rotar en los 3 ejes
            rotate(&z, &x, angle * 1.5);   // Z
            rotate(&y, &z, angle * 0.8);   // X
            rotate(&y, &x, angle * 2);     // Y

            // Proyección
            project(x, y, z, &px[i], &py[i]);
        }

        cleardevice();
        setcolor(color);

        // Dibujar aristas
        for (i = 0; i < numAristas; i++) {
            int a = aristas[i][0];
            int b = aristas[i][1];
            line(px[a], py[a], px[b], py[b]);
        }

        delay(40);
        angle += 0.05;
    }
}

int main() {
    int gd = DETECT, gm;
    initgraph(&gd, &gm, "C:\\Turboc3\\BGI");

    loadModel("C:\\test\\Show3D\\models\\model1.txt");
    drawModel(WHITE);
    loadModel("C:\\test\\Show3D\\models\\model2.txt");
    drawModel(RED);
    loadModel("C:\\test\\Show3D\\models\\model3.txt");
    drawModel(GREEN);
    loadModel("C:\\test\\Show3D\\models\\model4.txt");
    drawModel(BLUE);
    loadModel("C:\\test\\Show3D\\models\\model5.txt");
    drawModel(YELLOW);
    
    setcolor(RED);
    outtextxy(10, 10, "Fin del giro. Presiona una tecla...");
    getch();
    closegraph();
    return 0;
}
