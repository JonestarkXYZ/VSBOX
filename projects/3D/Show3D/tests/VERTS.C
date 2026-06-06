#include <stdio.h>
#include <conio.h>
#include <stdlib.h>
#include <string.h>

#define MODEL_PATH "C:\\projects\\3D\\Show3D\\models\\model1.txt"
#define WORD_SIZE 24
#define MAX_VERTICES 180

struct Vec3 {
    float x;
    float y;
    float z;
};

void waitExit(void) {
    /* Mantiene visible el resultado dentro de DOSBox. */
    printf("\nPresiona una tecla para salir...");
    getch();
}

int readVertexCount(FILE *archivo, int *cantidad) {
    char palabra[WORD_SIZE];

    /* Localiza el bloque VERTICES y devuelve su cantidad. */
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
    struct Vec3 vertices[MAX_VERTICES];
    int cantidad;
    int i;

    archivo = fopen(MODEL_PATH, "r");
    if (archivo == NULL) {
        printf("ERROR: no se pudo abrir el modelo:\n%s\n", MODEL_PATH);
        waitExit();
        return 1;
    }

    if (!readVertexCount(archivo, &cantidad)) {
        printf("ERROR: no se pudo leer el bloque VERTICES.\n");
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

    /* Lee cada coordenada como float y falla si una linea esta incompleta. */
    for (i = 0; i < cantidad; i++) {
        if (fscanf(archivo, "%f %f %f",
                   &vertices[i].x,
                   &vertices[i].y,
                   &vertices[i].z) != 3) {
            printf("ERROR: vertice %d incompleto o invalido.\n", i);
            fclose(archivo);
            waitExit();
            return 1;
        }
    }

    fclose(archivo);

    printf("OK: vertices leidos correctamente.\n");
    printf("Total: %d\n\n", cantidad);

    for (i = 0; i < cantidad; i++) {
        printf("V%d = {%.2f, %.2f, %.2f}\n",
               i,
               vertices[i].x,
               vertices[i].y,
               vertices[i].z);
    }

    waitExit();
    return 0;
}
