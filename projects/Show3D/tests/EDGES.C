#include <stdio.h>
#include <conio.h>
#include <stdlib.h>
#include <string.h>

#define MODEL_PATH "C:\\projects\\Show3D\\models\\model1.txt"
#define WORD_SIZE 24
#define MAX_VERTICES 180
#define MAX_ARISTAS 260

void waitExit(void) {
    /* Mantiene visible el resultado dentro de DOSBox. */
    printf("\nPresiona una tecla para salir...");
    getch();
}

int skipVertices(FILE *archivo, int cantidad) {
    int i;
    float x;
    float y;
    float z;

    /* Avanza sobre las coordenadas para llegar al bloque ARISTAS. */
    for (i = 0; i < cantidad; i++) {
        if (fscanf(archivo, "%f %f %f", &x, &y, &z) != 3) {
            return 0;
        }
    }

    return 1;
}

int main() {
    FILE *archivo;
    char palabra[WORD_SIZE];
    int numVertices;
    int numAristas;
    int a;
    int b;
    int i;
    int tieneVertices;
    int tieneAristas;

    archivo = fopen(MODEL_PATH, "r");
    if (archivo == NULL) {
        printf("ERROR: no se pudo abrir el modelo:\n%s\n", MODEL_PATH);
        waitExit();
        return 1;
    }

    numVertices = 0;
    numAristas = 0;
    tieneVertices = 0;
    tieneAristas = 0;

    while (fscanf(archivo, "%23s", palabra) == 1) {
        if (strcmp(palabra, "VERTICES") == 0) {
            if (fscanf(archivo, "%d", &numVertices) != 1) {
                printf("ERROR: VERTICES no tiene cantidad valida.\n");
                fclose(archivo);
                waitExit();
                return 1;
            }
            if (numVertices < 1 || numVertices > MAX_VERTICES) {
                printf("ERROR: cantidad de vertices fuera de rango: %d\n", numVertices);
                fclose(archivo);
                waitExit();
                return 1;
            }
            if (!skipVertices(archivo, numVertices)) {
                printf("ERROR: no se pudieron saltar los vertices.\n");
                fclose(archivo);
                waitExit();
                return 1;
            }
            tieneVertices = 1;
        } else if (strcmp(palabra, "ARISTAS") == 0) {
            if (!tieneVertices) {
                printf("ERROR: ARISTAS aparecio antes de VERTICES.\n");
                fclose(archivo);
                waitExit();
                return 1;
            }
            if (fscanf(archivo, "%d", &numAristas) != 1) {
                printf("ERROR: ARISTAS no tiene cantidad valida.\n");
                fclose(archivo);
                waitExit();
                return 1;
            }
            if (numAristas < 1 || numAristas > MAX_ARISTAS) {
                printf("ERROR: cantidad de aristas fuera de rango: %d\n", numAristas);
                fclose(archivo);
                waitExit();
                return 1;
            }

            /* Cada arista debe apuntar a vertices existentes. */
            for (i = 0; i < numAristas; i++) {
                if (fscanf(archivo, "%d %d", &a, &b) != 2) {
                    printf("ERROR: arista %d incompleta o invalida.\n", i);
                    fclose(archivo);
                    waitExit();
                    return 1;
                }
                if (a < 0 || b < 0 || a >= numVertices || b >= numVertices) {
                    printf("ERROR: arista %d fuera de rango: %d %d\n", i, a, b);
                    fclose(archivo);
                    waitExit();
                    return 1;
                }
                printf("A%d = {%d, %d}\n", i, a, b);
            }

            tieneAristas = 1;
            break;
        }
    }

    fclose(archivo);

    if (!tieneVertices || !tieneAristas) {
        printf("ERROR: faltan bloques VERTICES o ARISTAS.\n");
        waitExit();
        return 1;
    }

    printf("\nOK: aristas validas.\n");
    printf("Vertices: %d\n", numVertices);
    printf("Aristas: %d\n", numAristas);
    waitExit();
    return 0;
}
