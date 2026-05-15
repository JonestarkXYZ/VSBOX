#include <stdio.h>
#include <conio.h>
#include <stdlib.h>
#include <string.h>

#define MODEL_PATH "C:\\projects\\Show3D\\models\\model1.txt"
#define WORD_SIZE 24

void waitExit(void) {
    /* Mantiene visible el resultado dentro de DOSBox. */
    printf("\nPresiona una tecla para salir...");
    getch();
}

int main() {
    FILE *archivo;
    char palabra[WORD_SIZE];
    int cantidad;
    float x;
    float y;
    float z;

    archivo = fopen(MODEL_PATH, "r");
    if (archivo == NULL) {
        printf("ERROR: no se pudo abrir el modelo:\n%s\n", MODEL_PATH);
        waitExit();
        return 1;
    }

    /* Prueba minima de lectura float respetando el encabezado del formato. */
    if (fscanf(archivo, "%23s %d", palabra, &cantidad) != 2) {
        printf("ERROR: no se pudo leer el encabezado inicial.\n");
        fclose(archivo);
        waitExit();
        return 1;
    }

    if (strcmp(palabra, "VERTICES") != 0 || cantidad < 1) {
        printf("ERROR: encabezado VERTICES invalido: %s %d\n", palabra, cantidad);
        fclose(archivo);
        waitExit();
        return 1;
    }

    if (fscanf(archivo, "%f %f %f", &x, &y, &z) != 3) {
        printf("ERROR: no se pudo leer el primer vertice como floats.\n");
        fclose(archivo);
        waitExit();
        return 1;
    }

    fclose(archivo);

    printf("OK: primer vertice leido como float.\n");
    printf("V0 = {%.2f, %.2f, %.2f}\n", x, y, z);
    waitExit();
    return 0;
}
