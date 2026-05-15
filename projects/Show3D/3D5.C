#include <stdio.h>
#include <conio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_VERTICES 100

struct Vec3 {
    float x, y, z;
};

int main() {
    FILE* archivo;
    char palabra[20];
    int cantidad = 0;
    float verticeX;
    float verticeY;
    float verticeZ;
    struct Vec3 vertices[MAX_VERTICES];
    int i;

    archivo = fopen("C:\\projects\\Show3D\\models\\model1.txt", "r");
    if (!archivo) {
        printf("Error: no se pudo abrir el modelo\n");
        getch();
        return 1;
    }
    printf("PASO 1: Archivo abierto correctamente\n");

    while (fscanf(archivo, "%s", palabra) == 1) {
        if (strcmp(palabra, "VERTICES") == 0) {
            fscanf(archivo, "%d", &cantidad);
            break;
        }
    }

    printf("PASO 2: Vertices = %d\n", cantidad);

    if (cantidad < 1 || cantidad > MAX_VERTICES) {
        printf("Error: cantidad de vértices inválida: %d\n", cantidad);
        fclose(archivo);
        getch();
        return 1;
    }

    printf("PASO 3: Cantidad de vertices valida\n");
    getch();

    // PASO 4: Leer vértices con getch interno
    for (i = 0; i < cantidad; i++) {
        fscanf(archivo, "%f %f %f", &verticeX, &verticeY, &verticeZ);
        vertices[i].x = verticeX;
        vertices[i].y = verticeY;
        vertices[i].z = verticeZ;
        
        //fscanf(archivo, "%f", &vertice);
        //vertices[i].x = vertice;

        //fscanf(archivo, "%f", &vertice);
        //vertices[i].y = vertice;

        //fscanf(archivo, "%f", &vertice);
        //vertices[i].z = vertice;

        printf("Vertice %d: %.2f %.2f %.2f\n",i + 1, vertices[i].x, vertices[i].y, vertices[i].z);
    }

    printf("Todos los vertices leidos correctamente\n");
    getch();

    fclose(archivo);  // <<< Si se cierra aquí lo sabremos
    printf("Archivo cerrado correctamente\n");
    getch();

    // Mostrar resultados
    printf("CANTIDAD DE VERTICES: %d\n\n", cantidad);
    printf("COORDENADAS:\n");
    for (i = 0; i < cantidad; i++) {
        printf("CORD %d = {%.2f, %.2f, %.2f}\n",
            i + 1, vertices[i].x, vertices[i].y, vertices[i].z);
    }

    printf("\nPresiona una tecla para salir...");
    getch();
    return 0;
}
