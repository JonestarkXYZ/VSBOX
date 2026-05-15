#include <stdio.h>
#include <conio.h>
#include <stdlib.h>

int main() {
    FILE* archivo;
    int numero;

    archivo = fopen("C:\\projects\\Show3D\\models\\model1.txt", "r");
    if (!archivo) {
        printf("No se pudo abrir el modelo\n");
        getch();  // Esperar tecla para no cerrar
        return 1;
    }

    fscanf(archivo, "%d", &numero);
    fclose(archivo);

    printf("Numero leido: %d\n", numero);
    printf("Presiona una tecla para salir...");
    getch();  // Esperar tecla

    return 0;
}
