#include <stdio.h>
#include <conio.h>
#include <stdlib.h>
#include <string.h>

int main() {
    FILE* archivo;
    char palabra[20];
    int cantidad = 0;

    archivo = fopen("C:\\projects\\Show3D\\models\\model1.txt", "r");
    if (!archivo) {
        printf("No se pudo abrir el modelo\n");
        getch();
        return 1;
    }

    // Leer palabra y número
    while (fscanf(archivo, "%s", palabra) == 1) {
        if (strcmp(palabra, "VERTICES") == 0) {
            fscanf(archivo, "%d", &cantidad);
            break; // ya encontramos lo que necesitamos
        }
    }

    fclose(archivo);

    // Mostrar el resultado
    printf("CANTIDAD DE VERTICES: %d\n", cantidad);
    printf("Presiona una tecla para salir...");
    getch();
    return 0;
}
