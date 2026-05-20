/*
   SELHZ.C

   Selector de frecuencia para PLAYSB.C.
   Guarda la frecuencia elegida en C:\TURBOC3\SNDHZ.CFG.

   Importante: despues de cambiar los Hz, convierte otra vez el audio con
   convertir_audio_dosbox.py para que SOUND.RAW tenga la misma frecuencia.
*/

#include <conio.h>
#include <stdio.h>
#include <stdlib.h>

#define CFG_PATH "C:\\TURBOC3\\SNDHZ.CFG"
#define STATUS_PATH "C:\\TURBOC3\\SELHZ.TXT"
#define KEY_ESC 27
#define KEY_ENTER 13
#define KEY_EXTENDED 0
#define KEY_EXTENDED_ALT 224
#define KEY_ARROW_UP 72
#define KEY_ARROW_DOWN 80
#define OPTION_COUNT 4

unsigned long sampleRates[OPTION_COUNT] = {
    8000UL,
    11025UL,
    16000UL,
    22050UL
};

char *rateNotes[OPTION_COUNT] = {
    "Mas compatible, calidad baja",
    "Ligero, sonido tipo DOS clasico",
    "Balance recomendado",
    "Mas claro, exige mas al Direct DAC"
};

int readSavedIndex(void);
int indexFromRate(unsigned long rate);
void saveRate(unsigned long rate);
void writeStatus(unsigned long rate, int selectedIndex);
void drawMenu(int selectedIndex, int savedIndex);
int selectRateIndex(void);

int readSavedIndex(void) {
    FILE *cfg;
    unsigned long savedRate;
    int i;

    /* Si no hay configuracion, usa 16000 Hz como opcion recomendada. */
    cfg = fopen(CFG_PATH, "r");
    if (cfg == NULL) {
        return 2;
    }

    savedRate = 16000UL;
    if (fscanf(cfg, "%lu", &savedRate) != 1) {
        savedRate = 16000UL;
    }
    fclose(cfg);

    for (i = 0; i < OPTION_COUNT; i++) {
        if (sampleRates[i] == savedRate) {
            return i;
        }
    }

    return 2;
}

int indexFromRate(unsigned long rate) {
    int i;

    /* Permite usar SELHZ.EXE 16000 para pruebas no interactivas. */
    for (i = 0; i < OPTION_COUNT; i++) {
        if (sampleRates[i] == rate) {
            return i;
        }
    }

    return -1;
}

void saveRate(unsigned long rate) {
    FILE *cfg;

    /* PLAYSB.C solo necesita leer el primer numero del archivo. */
    cfg = fopen(CFG_PATH, "w");
    if (cfg == NULL) {
        printf("No se pudo guardar %s\n", CFG_PATH);
        return;
    }

    fprintf(cfg, "%lu\n", rate);
    fclose(cfg);
}

void writeStatus(unsigned long rate, int selectedIndex) {
    FILE *status;

    /* Archivo de diagnostico para confirmar la variable guardada. */
    status = fopen(STATUS_PATH, "w");
    if (status == NULL) {
        return;
    }

    fprintf(status, "SELHZ.C\n");
    fprintf(status, "selectedIndex=%d\n", selectedIndex);
    fprintf(status, "selectedRate=%lu\n", rate);
    fprintf(status, "config=%s\n", CFG_PATH);
    fclose(status);
}

void drawMenu(int selectedIndex, int savedIndex) {
    int i;

    clrscr();
    textcolor(WHITE);
    textbackground(BLACK);

    cprintf("VSBOX - Selector de Hz para PLAYSB.C\r\n");
    cprintf("Archivo: %s\r\n\r\n", CFG_PATH);

    cprintf("Usa flechas, 1-4, ENTER o ESC.\r\n");
    cprintf("Seleccion actual guardada: %lu Hz\r\n\r\n",
            sampleRates[savedIndex]);

    for (i = 0; i < OPTION_COUNT; i++) {
        if (i == selectedIndex) {
            textcolor(BLACK);
            textbackground(CYAN);
        } else {
            textcolor(LIGHTGRAY);
            textbackground(BLACK);
        }

        cprintf(" %d) %5lu Hz  %s ", i + 1, sampleRates[i], rateNotes[i]);
        textcolor(LIGHTGRAY);
        textbackground(BLACK);
        cprintf("\r\n");
    }

    textcolor(YELLOW);
    cprintf("\r\nNota: despues de cambiar Hz, reconvierte SOUND.RAW.\r\n");
    textcolor(LIGHTGRAY);
}

int selectRateIndex(void) {
    int savedIndex;
    int selectedIndex;
    int key;
    int scanCode;

    savedIndex = readSavedIndex();
    selectedIndex = savedIndex;
    drawMenu(selectedIndex, savedIndex);

    while (1) {
        key = getch();

        if (key == KEY_EXTENDED || key == KEY_EXTENDED_ALT) {
            scanCode = getch();

            if (scanCode == KEY_ARROW_UP) {
                selectedIndex--;
                if (selectedIndex < 0) {
                    selectedIndex = OPTION_COUNT - 1;
                }
                drawMenu(selectedIndex, savedIndex);
            } else if (scanCode == KEY_ARROW_DOWN) {
                selectedIndex++;
                if (selectedIndex >= OPTION_COUNT) {
                    selectedIndex = 0;
                }
                drawMenu(selectedIndex, savedIndex);
            }
        } else if (key >= '1' && key <= '4') {
            return key - '1';
        } else if (key == KEY_ENTER) {
            return selectedIndex;
        } else if (key == KEY_ESC) {
            return savedIndex;
        }
    }
}

int main(int argc, char *argv[]) {
    int selectedIndex;
    unsigned long requestedRate;

    if (argc > 1) {
        requestedRate = strtoul(argv[1], NULL, 10);
        selectedIndex = indexFromRate(requestedRate);
        if (selectedIndex < 0) {
            clrscr();
            printf("Hz no soportados: %lu\n", requestedRate);
            printf("Usa 8000, 11025, 16000 o 22050.\n");
            return 1;
        }
    } else {
        selectedIndex = selectRateIndex();
    }

    saveRate(sampleRates[selectedIndex]);
    writeStatus(sampleRates[selectedIndex], selectedIndex);

    clrscr();
    printf("Frecuencia guardada: %lu Hz\n", sampleRates[selectedIndex]);
    printf("Variable selectedIndex = %d\n", selectedIndex);
    printf("Archivo: %s\n", CFG_PATH);
    printf("Ahora convierte el audio y luego ejecuta PLAYSB.C.\n");

    if (argc <= 1) {
        printf("\nPresiona una tecla para salir...");
        getch();
    }

    return 0;
}
