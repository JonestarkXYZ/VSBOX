#include <windows.h>
#include <stdio.h>

int main() {
    HANDLE hComm;
    DCB dcbSerialParams = {0};  // Configuración del puerto
    COMMTIMEOUTS timeouts = {0};  // Tiempo de espera

    // Abrir el puerto COM3
    hComm = CreateFile("COM3", GENERIC_READ | GENERIC_WRITE, 0, 0, OPEN_EXISTING, 0, 0);

    if (hComm == INVALID_HANDLE_VALUE) {
        printf("Error al abrir el puerto COM3\n");
        return 1;
    }

    // Configuración de los parámetros del puerto serie
    dcbSerialParams.DCBlength = sizeof(dcbSerialParams);
    if (!GetCommState(hComm, &dcbSerialParams)) {
        printf("Error al obtener el estado del puerto\n");
        CloseHandle(hComm);
        return 1;
    }

    // Configurar el puerto con parámetros estándar (9600, 8 bits, sin paridad, 1 bit de parada)
    dcbSerialParams.BaudRate = CBR_9600;
    dcbSerialParams.ByteSize = 8;
    dcbSerialParams.StopBits = ONESTOPBIT;
    dcbSerialParams.Parity = NOPARITY;

    if (!SetCommState(hComm, &dcbSerialParams)) {
        printf("Error al configurar el puerto\n");
        CloseHandle(hComm);
        return 1;
    }

    // Configuración de timeouts
    timeouts.ReadIntervalTimeout = 50;
    timeouts.ReadTotalTimeoutConstant = 50;
    timeouts.ReadTotalTimeoutMultiplier = 10;
    timeouts.WriteTotalTimeoutConstant = 50;
    timeouts.WriteTotalTimeoutMultiplier = 10;

    if (!SetCommTimeouts(hComm, &timeouts)) {
        printf("Error al establecer los timeouts\n");
        CloseHandle(hComm);
        return 1;
    }

    // Enviar datos al puerto serial
    const char* data = "Hola desde COM3!";
    DWORD bytesWritten;
    if (!WriteFile(hComm, data, strlen(data), &bytesWritten, NULL)) {
        printf("Error al enviar datos\n");
    } else {
        printf("Datos enviados: %s\n", data);
    }

    // Cerrar el puerto serial
    CloseHandle(hComm);
    return 0;
}
