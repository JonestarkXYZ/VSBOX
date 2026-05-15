/* Genera la fecha y hora actual dentro de DOSBox para los logs de VSBOX. */
#include <stdio.h>
#include <dos.h>

int main(void)
{
    struct dosdate_t fecha;
    struct dostime_t hora;
    FILE *archivo;

    /* Escribe siempre en la carpeta de logs montada como C:\logs. */
    _dos_getdate(&fecha);
    _dos_gettime(&hora);

    archivo = fopen("C:\\logs\\DATETIME.TXT", "w");
    if (archivo == NULL) {
        return 1;
    }

    fprintf(archivo, "Fecha: %02d/%02d/%04d\n", fecha.day, fecha.month, fecha.year);
    fprintf(archivo, "Hora: %02d:%02d:%02d\n", hora.hour, hora.minute, hora.second);

    fclose(archivo);
    return 0;
}
