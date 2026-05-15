#include <stdio.h>
#include <conio.h>
#include <stdlib.h>
#include <string.h>

int main() {
    FILE* archivo;
    char palabra[20];
    int valor;
    int X = 0, Y = 0;

    archivo = fopen("C:\\projects\\Show3D\\models\\model1.txt", "r");
    if (!archivo) {
        printf("No se pudo abrir el modelo\n");
        getch();
        return 1;
    }

    while (fscanf(archivo, "%s %d", palabra, &valor) == 2) {
        if (strcmp(palabra, "X") == 0) {
            X = valor;
        } else if (strcmp(palabra, "Y") == 0) {
            Y = valor;
        } else {
            // Ignorar cualquier otra palabra
        }
    }

    fclose(archivo);

    printf("Valor de X: %d\n", X);
    printf("Valor de Y: %d\n", Y);

    printf("Presiona una tecla para salir...");
    getch();
    return 0;
}
