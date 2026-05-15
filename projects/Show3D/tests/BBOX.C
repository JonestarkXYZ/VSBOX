#include <stdio.h>
#include <conio.h>
#include <stdlib.h>
#include <string.h>

#define MODEL_PATH "C:\\projects\\Show3D\\models\\model1.txt"
#define WORD_SIZE 24
#define MAX_VERTICES 180

void waitExit(void) {
    /* Mantiene visible el resultado dentro de DOSBox. */
    printf("\nPresiona una tecla para salir...");
    getch();
}

int readVertexHeader(FILE *archivo, int *cantidad) {
    char palabra[WORD_SIZE];

    /* Lee hasta encontrar VERTICES y su cantidad asociada. */
    while (fscanf(archivo, "%23s", palabra) == 1) {
        if (strcmp(palabra, "VERTICES") == 0) {
            if (fscanf(archivo, "%d", cantidad) != 1) {
                return 0;
            }
            return 1;
        }
    }

    return 0;
}

int main() {
    FILE *archivo;
    int cantidad;
    int i;
    float x;
    float y;
    float z;
    float minX;
    float maxX;
    float minY;
    float maxY;
    float minZ;
    float maxZ;

    archivo = fopen(MODEL_PATH, "r");
    if (archivo == NULL) {
        printf("ERROR: no se pudo abrir el modelo:\n%s\n", MODEL_PATH);
        waitExit();
        return 1;
    }

    if (!readVertexHeader(archivo, &cantidad)) {
        printf("ERROR: no se pudo leer VERTICES.\n");
        fclose(archivo);
        waitExit();
        return 1;
    }

    if (cantidad < 1 || cantidad > MAX_VERTICES) {
        printf("ERROR: cantidad de vertices fuera de rango: %d\n", cantidad);
        fclose(archivo);
        waitExit();
        return 1;
    }

    if (fscanf(archivo, "%f %f %f", &x, &y, &z) != 3) {
        printf("ERROR: primer vertice incompleto o invalido.\n");
        fclose(archivo);
        waitExit();
        return 1;
    }

    minX = x;
    maxX = x;
    minY = y;
    maxY = y;
    minZ = z;
    maxZ = z;

    /* Calcula la caja de limites para comprobar coordenadas extremas. */
    for (i = 1; i < cantidad; i++) {
        if (fscanf(archivo, "%f %f %f", &x, &y, &z) != 3) {
            printf("ERROR: vertice %d incompleto o invalido.\n", i);
            fclose(archivo);
            waitExit();
            return 1;
        }

        if (x < minX) minX = x;
        if (x > maxX) maxX = x;
        if (y < minY) minY = y;
        if (y > maxY) maxY = y;
        if (z < minZ) minZ = z;
        if (z > maxZ) maxZ = z;
    }

    fclose(archivo);

    printf("OK: limites del modelo calculados.\n");
    printf("X: %.2f a %.2f\n", minX, maxX);
    printf("Y: %.2f a %.2f\n", minY, maxY);
    printf("Z: %.2f a %.2f\n", minZ, maxZ);
    printf("Tamano: %.2f x %.2f x %.2f\n",
           maxX - minX,
           maxY - minY,
           maxZ - minZ);

    waitExit();
    return 0;
}
