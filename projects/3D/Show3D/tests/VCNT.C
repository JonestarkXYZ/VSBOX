#include <stdio.h>
#include <conio.h>
#include <stdlib.h>
#include <string.h>

#define MODEL_PATH "C:\\projects\\3D\\Show3D\\models\\model1.txt"
#define WORD_SIZE 24
#define MAX_VERTICES 180

void waitExit(void) {
    /* Mantiene visible el resultado dentro de DOSBox. */
    printf("\nPresiona una tecla para salir...");
    getch();
}

int main() {
    FILE *archivo;
    char palabra[WORD_SIZE];
    int cantidad;
    int encontrado;

    archivo = fopen(MODEL_PATH, "r");
    if (archivo == NULL) {
        printf("ERROR: no se pudo abrir el modelo:\n%s\n", MODEL_PATH);
        waitExit();
        return 1;
    }

    encontrado = 0;
    cantidad = 0;

    /* Busca el bloque VERTICES sin asumir que siempre sera el primer token. */
    while (fscanf(archivo, "%23s", palabra) == 1) {
        if (strcmp(palabra, "VERTICES") == 0) {
            if (fscanf(archivo, "%d", &cantidad) != 1) {
                printf("ERROR: VERTICES no tiene una cantidad numerica.\n");
                fclose(archivo);
                waitExit();
                return 1;
            }
            encontrado = 1;
            break;
        }
    }

    fclose(archivo);

    if (!encontrado) {
        printf("ERROR: no se encontro el bloque VERTICES.\n");
        waitExit();
        return 1;
    }

    if (cantidad < 1 || cantidad > MAX_VERTICES) {
        printf("ERROR: cantidad de vertices fuera de rango: %d\n", cantidad);
        waitExit();
        return 1;
    }

    printf("OK: cantidad de vertices valida.\n");
    printf("VERTICES: %d\n", cantidad);
    waitExit();
    return 0;
}
