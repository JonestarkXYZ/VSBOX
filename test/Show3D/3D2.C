#include <stdio.h>
#include <conio.h>
#include <stdlib.h>

int main() {
    FILE* archivo;
    int numero;

    archivo = fopen("C:\\test\\Show3D\\models\\model1.txt", "r");
    if (!archivo) {
        printf("No se pudo abrir el modelo\n");
        getch();
        return 1;
    }

    printf("Numeros leidos del archivo:\n");

    // Leer hasta llegar al final del archivo
    while (fscanf(archivo, "%d", &numero) == 1) {
        printf("%d\n", numero);
    }

    fclose(archivo);

    printf("Fin del archivo. Presiona una tecla para salir...");
    getch();
    return 0;
}
