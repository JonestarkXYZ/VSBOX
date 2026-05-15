// FECHAHORA.C
#include <stdio.h>
#include <dos.h>

int main() {
    struct dosdate_t fecha;
    struct dostime_t hora;
    FILE *archivo;  // <-- MOVIDO ARRIBA

    _dos_getdate(&fecha);
    _dos_gettime(&hora);

    archivo = fopen("datetime.txt", "w");
    if (archivo != NULL) {
        fprintf(archivo, "Fecha: %02d/%02d/%04d\n", fecha.day, fecha.month, fecha.year);
        fprintf(archivo, "Hora: %02d:%02d:%02d\n", hora.hour, hora.minute, hora.second);
        fclose(archivo);
    }

    return 0;
}