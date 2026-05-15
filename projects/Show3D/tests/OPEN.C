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

    archivo = fopen(MODEL_PATH, "r");
    if (archivo == NULL) {
        printf("ERROR: no se pudo abrir el modelo:\n%s\n", MODEL_PATH);
        waitExit();
        return 1;
    }

    /* La primera prueba valida que el archivo inicia con el bloque VERTICES. */
    if (fscanf(archivo, "%23s %d", palabra, &cantidad) != 2) {
        printf("ERROR: no se pudo leer el encabezado inicial.\n");
        fclose(archivo);
        waitExit();
        return 1;
    }

    fclose(archivo);

    if (strcmp(palabra, "VERTICES") != 0 || cantidad <= 0) {
        printf("ERROR: encabezado invalido. Leido: %s %d\n", palabra, cantidad);
        waitExit();
        return 1;
    }

    printf("OK: archivo abierto correctamente.\n");
    printf("Encabezado: %s %d\n", palabra, cantidad);
    waitExit();
    return 0;
}
