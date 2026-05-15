#include <stdio.h>
#include <conio.h>
#include <stdlib.h>

int main() {
    FILE* archivo;
    float numero;

    archivo = fopen("C:\\projects\\Show3D\\models\\model1.txt", "r");
    if (!archivo) {
        printf("No se pudo abrir el modelo\n");
        getch();  // Esperar tecla para no cerrar
        return 1;
    }

    fscanf(archivo, "%f", &numero);
    fclose(archivo);

    printf("Numero leído (float): %.2f\n", numero);
    printf("Presiona una tecla para salir...");
    getch();  // Esperar tecla

    return 0;
}
