/*
   PLAYSB.C

   Reproductor basico para DOSBox + Turbo C 3 usando Sound Blaster.
   Formato esperado: RAW PCM unsigned 8-bit, mono.
   La frecuencia se lee desde C:\TURBOC3\SNDHZ.CFG.

   El conversor Python de este proyecto deja una copia como:
   C:\TURBOC3\SOUND.RAW

   Si SOUND.RAW no existe, el programa reproduce un tono de prueba generado
   en tiempo real para comprobar que Sound Blaster funciona en DOSBox.
*/

#include <conio.h>
#include <dos.h>
#include <stdio.h>
#include <stdlib.h>

#define SB_BASE             0x220
#define SB_DSP_READ         (SB_BASE + 0x0A)
#define SB_DSP_WRITE        (SB_BASE + 0x0C)
#define SB_DSP_WRITE_STATUS (SB_BASE + 0x0C)
#define SB_DSP_READ_STATUS  (SB_BASE + 0x0E)
#define SB_DSP_RESET        (SB_BASE + 0x06)

#define DSP_DIRECT_DAC      0x10
#define DSP_SPEAKER_ON      0xD1
#define DSP_SPEAKER_OFF     0xD3

#define PIT_CHANNEL0        0x40
#define PIT_CONTROL         0x43
#define PIT_HZ              1193182UL

#define DEFAULT_SAMPLE_RATE 16000UL
#define HZ_CONFIG_PATH      "C:\\TURBOC3\\SNDHZ.CFG"
#define PLAY_STATUS_PATH    "C:\\TURBOC3\\PLAYHZ.TXT"
#define BUFFER_SIZE         1024
#define KEY_ESC             27

unsigned char audioBuffer[BUFFER_SIZE];
unsigned long sampleRate = DEFAULT_SAMPLE_RATE;
unsigned long sampleStep = 0UL;
unsigned long sampleRemainderStep = 0UL;

unsigned long timerNow(void);
int waitUntil(unsigned long targetTicks);
void prepareSampleTiming(void);
void addSamplePeriod(unsigned long *targetTicks, unsigned long *remainder);
int isSupportedSampleRate(unsigned long rate);
unsigned long readSampleRateConfig(void);
void writePlaybackStatus(void);
void appendPlaybackFileStatus(const char *path, long fileSize);
long getFileSize(FILE *fp);
int sbReset(void);
int sbWrite(unsigned char value);
void sbSpeakerOn(void);
void sbSpeakerOff(void);
int sbDirectSample(unsigned char sample);
int userAbortRequested(void);
int playRawFile(const char *path);
int playDefaultRawFile(void);
void playGeneratedTone(unsigned int freqHz, unsigned int durationMs);
void playGeneratedDemo(void);
void playPcSpeakerDemo(void);

unsigned long timerNow(void) {
    volatile unsigned long far *biosTicks;
    unsigned long ticks;
    unsigned int count;
    unsigned char lo;
    unsigned char hi;

    /* Combina el contador BIOS de 18.2 Hz con el contador PIT para
       obtener una referencia de tiempo mas fina que delay(). */
    biosTicks = (volatile unsigned long far *)MK_FP(0x40, 0x6C);

    disable();
    ticks = *biosTicks;
    outportb(PIT_CONTROL, 0x00);
    lo = inportb(PIT_CHANNEL0);
    hi = inportb(PIT_CHANNEL0);
    enable();

    count = ((unsigned int)hi << 8) | lo;
    return (ticks << 16) + (unsigned long)(65535U - count);
}

int waitUntil(unsigned long targetTicks) {
    /* Espera activa corta para mantener el ritmo de muestras.
       El teclado se revisa fuera de este bucle para reducir carga. */
    while ((long)(timerNow() - targetTicks) < 0L) {
    }

    return 1;
}

void prepareSampleTiming(void) {
    /* Evita divisiones por muestra: Turbo C en DOS las hace costosas. */
    sampleStep = PIT_HZ / sampleRate;
    sampleRemainderStep = PIT_HZ % sampleRate;
}

void addSamplePeriod(unsigned long *targetTicks, unsigned long *remainder) {
    /* Solo sumas/comparaciones durante reproduccion para no trabar DOSBox. */
    *targetTicks += sampleStep;
    *remainder += sampleRemainderStep;

    if (*remainder >= sampleRate) {
        *targetTicks += 1UL;
        *remainder -= sampleRate;
    }
}

int isSupportedSampleRate(unsigned long rate) {
    /* Misma lista usada por SELHZ.C y convertir_audio_dosbox.py. */
    if (rate == 8000UL) {
        return 1;
    }
    if (rate == 11025UL) {
        return 1;
    }
    if (rate == 16000UL) {
        return 1;
    }
    if (rate == 22050UL) {
        return 1;
    }

    return 0;
}

unsigned long readSampleRateConfig(void) {
    FILE *cfg;
    unsigned long configuredRate;

    /* Si el selector no se ejecuto, conserva un valor estable. */
    cfg = fopen(HZ_CONFIG_PATH, "r");
    if (cfg == NULL) {
        return DEFAULT_SAMPLE_RATE;
    }

    configuredRate = DEFAULT_SAMPLE_RATE;
    if (fscanf(cfg, "%lu", &configuredRate) != 1) {
        configuredRate = DEFAULT_SAMPLE_RATE;
    }
    fclose(cfg);

    if (!isSupportedSampleRate(configuredRate)) {
        return DEFAULT_SAMPLE_RATE;
    }

    return configuredRate;
}

void writePlaybackStatus(void) {
    FILE *status;

    /* Archivo corto para comprobar desde Linux que PLAYSB.C leyo bien los Hz. */
    status = fopen(PLAY_STATUS_PATH, "w");
    if (status == NULL) {
        return;
    }

    fprintf(status, "PLAYSB.C\n");
    fprintf(status, "sampleRate=%lu\n", sampleRate);
    fprintf(status, "sampleStep=%lu\n", sampleStep);
    fprintf(status, "sampleRemainderStep=%lu\n", sampleRemainderStep);
    fprintf(status, "config=%s\n", HZ_CONFIG_PATH);
    fclose(status);
}

void appendPlaybackFileStatus(const char *path, long fileSize) {
    FILE *status;

    /* Agrega datos del RAW que realmente se logro abrir. */
    status = fopen(PLAY_STATUS_PATH, "a");
    if (status == NULL) {
        return;
    }

    fprintf(status, "rawPath=%s\n", path);
    fprintf(status, "rawBytes=%ld\n", fileSize);
    fprintf(status, "expectedSeconds=%ld\n", fileSize / (long)sampleRate);
    fclose(status);
}

long getFileSize(FILE *fp) {
    long current;
    long size;

    /* Calcula tamano sin cambiar la posicion final de lectura. */
    current = ftell(fp);
    fseek(fp, 0L, SEEK_END);
    size = ftell(fp);
    fseek(fp, current, SEEK_SET);

    return size;
}

int sbReset(void) {
    unsigned int timeout;

    /* Reset clasico del DSP: debe responder 0xAA si esta disponible. */
    outportb(SB_DSP_RESET, 1);
    delay(10);
    outportb(SB_DSP_RESET, 0);

    for (timeout = 0; timeout < 65535U; timeout++) {
        if (inportb(SB_DSP_READ_STATUS) & 0x80) {
            if (inportb(SB_DSP_READ) == 0xAA) {
                return 1;
            }
        }
    }

    return 0;
}

int sbWrite(unsigned char value) {
    unsigned int timeout;

    /* Espera a que el DSP acepte otro byte de comando/dato. */
    for (timeout = 0; timeout < 65535U; timeout++) {
        if ((inportb(SB_DSP_WRITE_STATUS) & 0x80) == 0) {
            outportb(SB_DSP_WRITE, value);
            return 1;
        }
    }

    return 0;
}

void sbSpeakerOn(void) {
    sbWrite(DSP_SPEAKER_ON);
}

void sbSpeakerOff(void) {
    sbWrite(DSP_SPEAKER_OFF);
}

int sbDirectSample(unsigned char sample) {
    /* Direct DAC: comando 0x10 seguido de una muestra unsigned 8-bit. */
    if (!sbWrite(DSP_DIRECT_DAC)) {
        return 0;
    }

    return sbWrite(sample);
}

int userAbortRequested(void) {
    int key;

    if (!kbhit()) {
        return 0;
    }

    key = getch();

    /* Descarta el segundo byte de teclas extendidas si aparece. */
    if ((key == 0 || key == 224) && kbhit()) {
        getch();
    }

    return key == KEY_ESC;
}

int playRawFile(const char *path) {
    FILE *fp;
    unsigned int got;
    unsigned int i;
    unsigned long targetTicks;
    unsigned long remainder;
    long fileSize;

    fp = fopen(path, "rb");
    if (fp == NULL) {
        return 0;
    }

    fileSize = getFileSize(fp);
    appendPlaybackFileStatus(path, fileSize);
    printf("Reproduciendo: %s\n", path);
    printf("Bytes RAW: %ld, duracion esperada: %ld s aprox.\n",
           fileSize, fileSize / (long)sampleRate);
    printf("ESC cancela la reproduccion.\n\n");

    targetTicks = timerNow();
    remainder = 0UL;

    while ((got = (unsigned int)fread(audioBuffer, 1, BUFFER_SIZE, fp)) > 0) {
        for (i = 0; i < got; i++) {
            if ((i & 63) == 0 && userAbortRequested()) {
                fclose(fp);
                return 2;
            }

            if (!waitUntil(targetTicks)) {
                fclose(fp);
                return 2;
            }

            if (!sbDirectSample(audioBuffer[i])) {
                fclose(fp);
                return 3;
            }

            addSamplePeriod(&targetTicks, &remainder);
        }
    }

    fclose(fp);
    return 1;
}

int playDefaultRawFile(void) {
    const char *paths[] = {
        "SOUND.RAW",
        "C:\\TURBOC3\\SOUND.RAW",
        "C:\\SOUND.RAW",
        NULL
    };
    int i;
    int result;

    /* Se prueban rutas cortas compatibles con DOS 8.3. */
    for (i = 0; paths[i] != NULL; i++) {
        result = playRawFile(paths[i]);
        if (result != 0) {
            return result;
        }
    }

    return 0;
}

void playGeneratedTone(unsigned int freqHz, unsigned int durationMs) {
    unsigned long totalSamples;
    unsigned long sampleIndex;
    unsigned long targetTicks;
    unsigned long remainder;
    unsigned int halfPeriod;
    unsigned int halfCounter;
    int high;
    unsigned char sample;

    /* Onda cuadrada simple para prueba cuando no hay archivo convertido. */
    totalSamples = (sampleRate * (unsigned long)durationMs) / 1000UL;
    halfPeriod = (unsigned int)(sampleRate / ((unsigned long)freqHz * 2UL));
    if (halfPeriod == 0) {
        halfPeriod = 1;
    }

    targetTicks = timerNow();
    remainder = 0UL;
    halfCounter = 0;
    high = 1;

    for (sampleIndex = 0; sampleIndex < totalSamples; sampleIndex++) {
        if ((sampleIndex & 63UL) == 0UL && userAbortRequested()) {
            return;
        }

        if (!waitUntil(targetTicks)) {
            return;
        }

        sample = high ? 210 : 46;
        if (!sbDirectSample(sample)) {
            return;
        }

        halfCounter++;
        if (halfCounter >= halfPeriod) {
            halfCounter = 0;
            high = !high;
        }

        addSamplePeriod(&targetTicks, &remainder);
    }
}

void playGeneratedDemo(void) {
    printf("No se encontro SOUND.RAW.\n");
    printf("Reproduciendo tono de prueba generado por el programa.\n\n");

    playGeneratedTone(440, 250);
    playGeneratedTone(660, 250);
    playGeneratedTone(880, 350);
}

void playPcSpeakerDemo(void) {
    /* Respaldo minimo si Sound Blaster no responde en DOSBox. */
    sound(440);
    delay(220);
    sound(660);
    delay(220);
    sound(880);
    delay(320);
    nosound();
}

int main(int argc, char *argv[]) {
    int result;

    clrscr();
    sampleRate = readSampleRateConfig();
    prepareSampleTiming();
    writePlaybackStatus();

    printf("VSBOX - Prueba de sonido para DOSBox\n");
    printf("Sound Blaster: A220 I7 D1, RAW unsigned 8-bit mono %lu Hz\n\n",
           sampleRate);
    printf("Variable sampleRate = %lu Hz\n", sampleRate);
    printf("Archivo de Hz: %s\n", HZ_CONFIG_PATH);
    printf("El RAW debe haber sido convertido a esos mismos Hz.\n\n");

    if (!sbReset()) {
        printf("No se detecto Sound Blaster en 0x220.\n");
        printf("Probando sonido por PC speaker como respaldo.\n");
        playPcSpeakerDemo();
        return 1;
    }

    sbSpeakerOn();

    if (argc > 1) {
        result = playRawFile(argv[1]);
    } else {
        result = playDefaultRawFile();
    }

    if (result == 0) {
        playGeneratedDemo();
    } else if (result == 2) {
        printf("Reproduccion cancelada por el usuario.\n");
    } else if (result == 3) {
        printf("Error escribiendo al DSP de Sound Blaster.\n");
    } else {
        printf("Reproduccion finalizada.\n");
    }

    sbSpeakerOff();
    delay(250);
    return 0;
}
