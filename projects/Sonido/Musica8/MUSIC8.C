/*
   MUSIC8.C

   Reproductor de musica 8-bit escrita a mano para DOSBox + Turbo C 3.
   Lee un archivo MUSIC.TXT con secciones como:

       Piano[C4,4,D4,4,E4,4,MUTE,4]
       Bajo[C3,2,G2,2]
       Bateria[KICK,4,HAT,8,SNARE,4,HAT,8]
       Frecuencia[C4,4,MUTE,4,E4,4,G4,4]

   Cada seccion es una pista. Todas las pistas empiezan al mismo tiempo.
   La duracion es un divisor del pulso del tempo:

       1 = pulso completo
       2 = medio pulso
       4 = cuarto de pulso
       8 = octavo de pulso
      16 = dieciseisavo de pulso

   Frecuencia acepta notas musicales y valores numericos en Hz.
   El valor 0 o MUTE es silencio.
   El token LOOP dentro de una pista marca desde donde repetir cuando la
   cancion termina. Tambien se puede forzar bucle con LOOP_SONG.
   El sonido sale por Sound Blaster Direct DAC, unsigned 8-bit.
*/

#include <conio.h>
#include <ctype.h>
#include <dos.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

#define SAMPLE_RATE         11025UL
#define TEMPO_BPM           60
#define TEMPO_SPEED_PERCENT 100UL
#define PITCH_CORRECT_PCT   100UL
#define LOOP_SONG           0

#define MAX_TRACKS          6
#define MAX_EVENTS          384
#define NAME_LEN            16
#define TOKEN_LEN           24
#define DEFAULT_OCTAVE      6
#define MAX_OCTAVE          8
#define PHASE_SCALE         65536UL
#define RELEASE_SAMPLES     96UL
#define ATTACK_SAMPLES      8UL
#define ENABLE_CATCHUP      0
#define MAX_CATCHUP_SAMPLES 96U
#define FREQ_VOLUME         46U
#define KEY_ESC             27

#define INST_PIANO          1
#define INST_BASS           2
#define INST_DRUM           3
#define INST_LEAD           4
#define INST_FREQ           5

#define NOTE_MUTE           0
#define DRUM_KICK          -1
#define DRUM_SNARE         -2
#define DRUM_HAT           -3

typedef struct {
    int freqHz;
    unsigned int durationDiv;
} MusicEvent;

typedef struct {
    char name[NAME_LEN];
    int instrument;
    unsigned int eventCount;
    unsigned int loopStartIndex;
    int loopEnabled;
    MusicEvent events[MAX_EVENTS];
} MusicTrack;

typedef struct {
    unsigned int eventIndex;
    unsigned long samplesLeft;
    unsigned long totalSamples;
    unsigned long samplePos;
    unsigned long phase;
    unsigned long step;
    unsigned int noise;
    int currentFreq;
    int active;
} VoiceState;

MusicTrack tracks[MAX_TRACKS];
VoiceState voices[MAX_TRACKS];
int trackCount = 0;

signed char freqWave[256];
unsigned long sampleStep = 0UL;
unsigned long sampleRemainderStep = 0UL;
unsigned long beatSamples = 0UL;
unsigned long skippedSamplesTotal = 0UL;
int songLoopEnabled = 0;
char loadedMusicPath[80];

unsigned int noteTable[MAX_OCTAVE + 1][12] = {
    {  16,   17,   18,   19,   21,   22,   23,   25,   26,   28,   29,   31 },
    {  33,   35,   37,   39,   41,   44,   46,   49,   52,   55,   58,   62 },
    {  65,   69,   73,   78,   82,   87,   92,   98,  104,  110,  117,  123 },
    { 131,  139,  147,  156,  165,  175,  185,  196,  208,  220,  233,  247 },
    { 262,  277,  294,  311,  330,  349,  370,  392,  415,  440,  466,  494 },
    { 523,  554,  587,  622,  659,  698,  740,  784,  831,  880,  932,  988 },
    {1047, 1109, 1175, 1245, 1319, 1397, 1480, 1568, 1661, 1760, 1865, 1976 },
    {2093, 2217, 2349, 2489, 2637, 2794, 2960, 3136, 3322, 3520, 3729, 3951 },
    {4186, 4435, 4699, 4978, 5274, 5588, 5920, 6272, 6645, 7040, 7459, 7902 }
};

char *musicPaths[] = {
    "MUSIC.TXT",
    "C:\\TURBOC3\\MUSIC.TXT",
    "C:\\PROJECTS\\SONIDO\\MUSICA8\\MUSIC.TXT",
    NULL
};

unsigned long timerNow(void);
void prepareTiming(void);
void addSamplePeriod(unsigned long *targetTicks, unsigned long *remainder);
void waitUntil(unsigned long targetTicks);
int userAbortRequested(void);

int sbReset(void);
int sbWrite(unsigned char value);
void sbSpeakerOn(void);
void sbSpeakerOff(void);
int sbDirectSample(unsigned char sample);

int equalsIgnoreCase(const char *a, const char *b);
void copyUpperTrimmed(char *dst, const char *src, int maxLen);
void trimToken(char *token);
int readCleanChar(FILE *fp);
int readNonSpace(FILE *fp);
void readSectionName(FILE *fp, int firstChar, char *name);
int readListToken(FILE *fp, char *token, int maxLen);
int classifyInstrument(const char *name);
int noteIndexFromLetter(int letter);
int parseNoteToken(const char *token, int instrument, int *freqHz);
int parseTrackEvents(FILE *fp, MusicTrack *track);
int loadMusicFromPath(const char *path);
int loadMusicFile(const char *requestedPath);
void printLoadedTracks(void);

unsigned long durationToSamples(unsigned int durationDiv);
unsigned long freqToStep(int freqHz);
void startNextEvent(int trackIndex);
void resetVoicesFromStart(void);
void resetVoicesFromLoop(void);
unsigned int envelopeVolume(unsigned int baseVolume, VoiceState *voice,
                            int fastDecay);
void initFrequencyWave(void);
void advanceVoicePhase(VoiceState *voice);
int frequencyWaveFromPhase(unsigned long phase, unsigned int volume);
int renderToneSample(MusicTrack *track, VoiceState *voice);
int renderDrumSample(VoiceState *voice);
int renderTrackSample(int trackIndex);
int updateActiveEvents(void);
void advanceTrackSilent(int trackIndex);
int catchUpLateSamples(unsigned long *targetTicks, unsigned long *remainder);
int playSongOnce(void);
void playPcSpeakerFallback(void);

unsigned long timerNow(void) {
    volatile unsigned long far *biosTicks;
    unsigned long ticks;
    unsigned int count;
    unsigned char lo;
    unsigned char hi;

    /* Combina el contador BIOS con el PIT para temporizar cada muestra. */
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

void prepareTiming(void) {
    /* Precalcula pasos para evitar divisiones dentro del bucle principal. */
    sampleStep = PIT_HZ / SAMPLE_RATE;
    sampleRemainderStep = PIT_HZ % SAMPLE_RATE;

    /* TEMPO_SPEED_PERCENT permite ajustar velocidad sin cambiar afinacion.
       100 = normal, 125 = 25% mas rapido, 80 = 20% mas lento. */
    beatSamples = (SAMPLE_RATE * 60UL * 100UL) /
                  ((unsigned long)TEMPO_BPM * TEMPO_SPEED_PERCENT);
    if (beatSamples == 0UL) {
        beatSamples = 1UL;
    }

    initFrequencyWave();
}

void addSamplePeriod(unsigned long *targetTicks, unsigned long *remainder) {
    /* Avanza el reloj de salida manteniendo el error acumulado controlado. */
    *targetTicks += sampleStep;
    *remainder += sampleRemainderStep;

    if (*remainder >= SAMPLE_RATE) {
        *targetTicks += 1UL;
        *remainder -= SAMPLE_RATE;
    }
}

void waitUntil(unsigned long targetTicks) {
    /* Espera activa corta: Direct DAC necesita ritmo estable por muestra. */
    while ((long)(timerNow() - targetTicks) < 0L) {
    }
}

int userAbortRequested(void) {
    int key;

    if (!kbhit()) {
        return 0;
    }

    key = getch();

    /* Descarta el segundo byte de teclas extendidas. */
    if ((key == 0 || key == 224) && kbhit()) {
        getch();
    }

    return key == KEY_ESC;
}

int sbReset(void) {
    unsigned int timeout;

    /* Reset clasico del DSP: debe responder 0xAA. */
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

    /* Espera a que el DSP pueda recibir otro comando o dato. */
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

int equalsIgnoreCase(const char *a, const char *b) {
    int ca;
    int cb;

    /* Comparacion ASCII simple para nombres de instrumentos y tokens. */
    while (*a != '\0' && *b != '\0') {
        ca = toupper((unsigned char)*a);
        cb = toupper((unsigned char)*b);
        if (ca != cb) {
            return 0;
        }
        a++;
        b++;
    }

    return *a == '\0' && *b == '\0';
}

void copyUpperTrimmed(char *dst, const char *src, int maxLen) {
    int i;

    /* Normaliza tokens como " c#6 " a "C#6". */
    strncpy(dst, src, maxLen - 1);
    dst[maxLen - 1] = '\0';
    trimToken(dst);

    for (i = 0; dst[i] != '\0'; i++) {
        dst[i] = (char)toupper((unsigned char)dst[i]);
    }
}

void trimToken(char *token) {
    int start;
    int end;
    int i;

    /* Quita espacios alrededor sin tocar el contenido musical del token. */
    if (token[0] == '\0') {
        return;
    }

    start = 0;
    while (token[start] != '\0' && isspace((unsigned char)token[start])) {
        start++;
    }

    end = (int)strlen(token) - 1;
    while (end >= start && isspace((unsigned char)token[end])) {
        end--;
    }

    if (start > 0) {
        i = 0;
        while (start <= end) {
            token[i] = token[start];
            i++;
            start++;
        }
        token[i] = '\0';
    } else {
        token[end + 1] = '\0';
    }
}

int readCleanChar(FILE *fp) {
    int c;
    int next;

    /* Permite comentarios // dentro de MUSIC.TXT. */
    while (1) {
        c = fgetc(fp);
        if (c != '/') {
            return c;
        }

        next = fgetc(fp);
        if (next == '/') {
            while (c != EOF && c != '\n') {
                c = fgetc(fp);
            }
        } else {
            if (next != EOF) {
                ungetc(next, fp);
            }
            return c;
        }
    }
}

int readNonSpace(FILE *fp) {
    int c;

    /* Busca el siguiente caracter significativo fuera de espacios. */
    do {
        c = readCleanChar(fp);
    } while (c != EOF && isspace(c));

    return c;
}

void readSectionName(FILE *fp, int firstChar, char *name) {
    int c;
    int len;

    /* Lee nombres como Piano, Bajo, Bateria o Lead. */
    len = 0;
    c = firstChar;
    while (c != EOF && (isalnum(c) || c == '_')) {
        if (len < NAME_LEN - 1) {
            name[len] = (char)c;
            len++;
        }
        c = readCleanChar(fp);
    }
    name[len] = '\0';

    if (c != EOF) {
        ungetc(c, fp);
    }
}

int readListToken(FILE *fp, char *token, int maxLen) {
    int c;
    int len;

    /* Lee un token hasta coma o cierre de seccion. */
    len = 0;
    token[0] = '\0';

    while (1) {
        c = readCleanChar(fp);

        if (c == EOF || c == ',' || c == ']') {
            token[len] = '\0';
            trimToken(token);
            return c;
        }

        if (len < maxLen - 1) {
            token[len] = (char)c;
            len++;
        }
    }
}

int classifyInstrument(const char *name) {
    /* El nombre de la seccion decide el sintetizador usado. */
    if (equalsIgnoreCase(name, "BAJO") || equalsIgnoreCase(name, "BASS")) {
        return INST_BASS;
    }

    if (equalsIgnoreCase(name, "BATERIA") ||
        equalsIgnoreCase(name, "DRUM") ||
        equalsIgnoreCase(name, "DRUMS")) {
        return INST_DRUM;
    }

    if (equalsIgnoreCase(name, "LEAD") ||
        equalsIgnoreCase(name, "SYNTH") ||
        equalsIgnoreCase(name, "MELODIA")) {
        return INST_LEAD;
    }

    if (equalsIgnoreCase(name, "FRECUENCIA") ||
        equalsIgnoreCase(name, "FREQ") ||
        equalsIgnoreCase(name, "HZ") ||
        equalsIgnoreCase(name, "TONO")) {
        return INST_FREQ;
    }

    return INST_PIANO;
}

int noteIndexFromLetter(int letter) {
    /* Orden cromatico: C, C#, D, D#, E, F, F#, G, G#, A, A#, B. */
    switch (toupper(letter)) {
        case 'C': return 0;
        case 'D': return 2;
        case 'E': return 4;
        case 'F': return 5;
        case 'G': return 7;
        case 'A': return 9;
        case 'B': return 11;
    }

    return -1;
}

int parseNoteToken(const char *token, int instrument, int *freqHz) {
    char upper[TOKEN_LEN];
    int index;
    int octave;
    int pos;

    copyUpperTrimmed(upper, token, TOKEN_LEN);

    if (isdigit((unsigned char)upper[0])) {
        /* Permite escribir frecuencias puras: 440,4 o 0,4 para silencio. */
        *freqHz = atoi(upper);
        if (*freqHz < 0 || *freqHz > 8000) {
            return 0;
        }
        return 1;
    }

    /* Silencio explicito compatible con los ejemplos del usuario. */
    if (equalsIgnoreCase(upper, "MUTE") ||
        equalsIgnoreCase(upper, "REST") ||
        equalsIgnoreCase(upper, "SILENCIO") ||
        equalsIgnoreCase(upper, "R")) {
        *freqHz = NOTE_MUTE;
        return 1;
    }

    /* Nombres practicos para percusion dentro de la pista Bateria. */
    if (instrument == INST_DRUM) {
        if (equalsIgnoreCase(upper, "KICK") ||
            equalsIgnoreCase(upper, "BOMBO") ||
            equalsIgnoreCase(upper, "BD")) {
            *freqHz = DRUM_KICK;
            return 1;
        }

        if (equalsIgnoreCase(upper, "SNARE") ||
            equalsIgnoreCase(upper, "CAJA") ||
            equalsIgnoreCase(upper, "SD")) {
            *freqHz = DRUM_SNARE;
            return 1;
        }

        if (equalsIgnoreCase(upper, "HAT") ||
            equalsIgnoreCase(upper, "HH") ||
            equalsIgnoreCase(upper, "PLATILLO")) {
            *freqHz = DRUM_HAT;
            return 1;
        }
    }

    index = noteIndexFromLetter(upper[0]);
    if (index < 0) {
        return 0;
    }

    pos = 1;
    if (upper[pos] == '#') {
        index++;
        pos++;
    } else if (upper[pos] == 'S') {
        /* Alias DOS-friendly: CS6 equivale a C#6, DS6 a D#6. */
        index++;
        pos++;
    }

    if (index >= 12) {
        return 0;
    }

    if (upper[pos] == '\0') {
        octave = DEFAULT_OCTAVE;
    } else {
        octave = atoi(&upper[pos]);
    }

    if (octave < 0 || octave > MAX_OCTAVE) {
        return 0;
    }

    *freqHz = (int)noteTable[octave][index];
    return 1;
}

int parseTrackEvents(FILE *fp, MusicTrack *track) {
    char noteToken[TOKEN_LEN];
    char durationToken[TOKEN_LEN];
    int delimiter;
    int endDelimiter;
    int freqHz;
    int durationDiv;

    /* La lista se lee como pares nota,duracion hasta encontrar ']'. */
    while (1) {
        delimiter = readListToken(fp, noteToken, TOKEN_LEN);

        if (delimiter == EOF) {
            printf("Error: seccion %s sin cierre ].\n", track->name);
            return 0;
        }

        if (noteToken[0] == '\0' && delimiter == ']') {
            return 1;
        }

        if (noteToken[0] == '\0' && delimiter == ',') {
            continue;
        }

        if (equalsIgnoreCase(noteToken, "LOOP")) {
            /* LOOP no suena: marca el evento al que se vuelve al terminar. */
            track->loopStartIndex = track->eventCount;
            track->loopEnabled = 1;
            songLoopEnabled = 1;

            if (delimiter == ']') {
                return 1;
            }
            if (delimiter == ',') {
                continue;
            }
        }

        if (delimiter != ',') {
            printf("Error: falta duracion despues de %s en %s.\n",
                   noteToken, track->name);
            return 0;
        }

        endDelimiter = readListToken(fp, durationToken, TOKEN_LEN);
        if (durationToken[0] == '\0') {
            printf("Error: duracion vacia en %s.\n", track->name);
            return 0;
        }

        if (!parseNoteToken(noteToken, track->instrument, &freqHz)) {
            printf("Error: nota no valida '%s' en %s.\n",
                   noteToken, track->name);
            return 0;
        }

        durationDiv = atoi(durationToken);
        if (durationDiv <= 0) {
            printf("Error: duracion no valida '%s' en %s.\n",
                   durationToken, track->name);
            return 0;
        }

        if (track->eventCount >= MAX_EVENTS) {
            printf("Error: demasiados eventos en %s. Maximo: %u.\n",
                   track->name, MAX_EVENTS);
            return 0;
        }

        track->events[track->eventCount].freqHz = freqHz;
        track->events[track->eventCount].durationDiv =
            (unsigned int)durationDiv;
        track->eventCount++;

        if (endDelimiter == ']') {
            return 1;
        }

        if (endDelimiter == EOF) {
            printf("Error: seccion %s sin cierre ].\n", track->name);
            return 0;
        }
    }
}

int loadMusicFromPath(const char *path) {
    FILE *fp;
    int c;
    MusicTrack *track;

    fp = fopen(path, "r");
    if (fp == NULL) {
        return 0;
    }

    trackCount = 0;
    songLoopEnabled = 0;
    strcpy(loadedMusicPath, path);

    /* Busca bloques Nombre[ ... ] en cualquier lugar del archivo. */
    while ((c = readNonSpace(fp)) != EOF) {
        if (!isalpha(c)) {
            continue;
        }

        if (trackCount >= MAX_TRACKS) {
            printf("Error: demasiadas pistas. Maximo: %u.\n", MAX_TRACKS);
            fclose(fp);
            return -1;
        }

        track = &tracks[trackCount];
        memset(track, 0, sizeof(MusicTrack));
        readSectionName(fp, c, track->name);

        c = readNonSpace(fp);
        if (c != '[') {
            continue;
        }

        track->instrument = classifyInstrument(track->name);
        if (!parseTrackEvents(fp, track)) {
            fclose(fp);
            return -1;
        }

        trackCount++;
    }

    fclose(fp);
    return trackCount > 0 ? 1 : -1;
}

int loadMusicFile(const char *requestedPath) {
    int i;
    int result;

    if (requestedPath != NULL) {
        return loadMusicFromPath(requestedPath);
    }

    /* Prueba rutas utiles para ejecutar desde C:\TURBOC3 o desde proyecto. */
    for (i = 0; musicPaths[i] != NULL; i++) {
        result = loadMusicFromPath(musicPaths[i]);
        if (result != 0) {
            return result;
        }
    }

    return 0;
}

void printLoadedTracks(void) {
    int i;
    char *typeName;

    /* Resumen corto para verificar que el parser leyo las secciones. */
    printf("Archivo: %s\n", loadedMusicPath);
    printf("Tempo: %u BPM, velocidad: %lu%%, pitch: %lu%%\n",
           TEMPO_BPM, TEMPO_SPEED_PERCENT, PITCH_CORRECT_PCT);
    printf("Sample rate: %lu Hz, loop: %s\n\n",
           SAMPLE_RATE, (songLoopEnabled || LOOP_SONG) ? "SI" : "NO");

    for (i = 0; i < trackCount; i++) {
        typeName = "Piano";
        if (tracks[i].instrument == INST_BASS) {
            typeName = "Bajo";
        } else if (tracks[i].instrument == INST_DRUM) {
            typeName = "Bateria";
        } else if (tracks[i].instrument == INST_LEAD) {
            typeName = "Lead";
        } else if (tracks[i].instrument == INST_FREQ) {
            typeName = "Frecuencia";
        }

        printf("%s: %u eventos (%s)",
               tracks[i].name, tracks[i].eventCount, typeName);

        if (tracks[i].loopEnabled) {
            printf(", LOOP en evento %u", tracks[i].loopStartIndex);
        }

        printf("\n");
    }
}

unsigned long durationToSamples(unsigned int durationDiv) {
    unsigned long samples;

    /* La duracion es una division del pulso definido por TEMPO_BPM. */
    if (durationDiv == 0U) {
        durationDiv = 1U;
    }

    samples = beatSamples / (unsigned long)durationDiv;
    if (samples == 0UL) {
        samples = 1UL;
    }

    return samples;
}

unsigned long freqToStep(int freqHz) {
    unsigned long step;

    /* Paso de fase para osciladores de 16 bits. */
    if (freqHz <= 0) {
        return 0UL;
    }

    /* PITCH_CORRECT_PCT corrige afinacion si DOSBox reproduce grave/agudo.
       100 = normal, 200 = una octava arriba, 50 = una octava abajo. */
    step = ((unsigned long)freqHz * PHASE_SCALE) / SAMPLE_RATE;
    step = (step * PITCH_CORRECT_PCT) / 100UL;
    if (step == 0UL) {
        step = 1UL;
    }

    return step;
}

void startNextEvent(int trackIndex) {
    MusicTrack *track;
    VoiceState *voice;
    MusicEvent *event;

    /* Avanza una pista al siguiente evento musical. */
    track = &tracks[trackIndex];
    voice = &voices[trackIndex];

    if (voice->eventIndex >= track->eventCount) {
        voice->active = 0;
        voice->samplesLeft = 0UL;
        return;
    }

    event = &track->events[voice->eventIndex];
    voice->eventIndex++;

    voice->currentFreq = event->freqHz;
    voice->totalSamples = durationToSamples(event->durationDiv);
    voice->samplesLeft = voice->totalSamples;
    voice->samplePos = 0UL;
    voice->phase = 0UL;
    voice->step = freqToStep(event->freqHz);
    voice->active = 1;

    if (track->instrument == INST_DRUM) {
        if (event->freqHz == DRUM_KICK) {
            voice->step = freqToStep(85);
        } else if (event->freqHz > 0) {
            voice->step = freqToStep(event->freqHz);
        }
    }
}

void resetVoicesFromStart(void) {
    int i;

    /* Reinicia todas las pistas desde el inicio de la cancion. */
    for (i = 0; i < MAX_TRACKS; i++) {
        memset(&voices[i], 0, sizeof(VoiceState));
        voices[i].noise = (unsigned int)(0xACE1U + (unsigned int)(i * 197));
    }

    for (i = 0; i < trackCount; i++) {
        startNextEvent(i);
    }
}

void resetVoicesFromLoop(void) {
    int i;

    /* Reinicia desde LOOP en cada pista. Si una pista no tiene LOOP propio,
       vuelve al inicio para mantenerla activa durante el bucle. */
    for (i = 0; i < MAX_TRACKS; i++) {
        memset(&voices[i], 0, sizeof(VoiceState));
        voices[i].noise = (unsigned int)(0xACE1U + (unsigned int)(i * 197));
    }

    for (i = 0; i < trackCount; i++) {
        if (tracks[i].loopEnabled) {
            voices[i].eventIndex = tracks[i].loopStartIndex;
        } else {
            voices[i].eventIndex = 0;
        }

        startNextEvent(i);
    }
}

unsigned int envelopeVolume(unsigned int baseVolume, VoiceState *voice,
                            int fastDecay) {
    unsigned long env;

    /* Ataque y salida corta para reducir clicks entre notas. */
    env = baseVolume;

    if (fastDecay && voice->totalSamples > 0UL) {
        env = ((unsigned long)baseVolume * voice->samplesLeft) /
              voice->totalSamples;
        if (env < (unsigned long)(baseVolume / 4U)) {
            env = baseVolume / 4U;
        }
    }

    if (voice->samplePos < ATTACK_SAMPLES) {
        env = (env * voice->samplePos) / ATTACK_SAMPLES;
    }

    if (voice->samplesLeft < RELEASE_SAMPLES) {
        env = (env * voice->samplesLeft) / RELEASE_SAMPLES;
    }

    if (env > 255UL) {
        env = 255UL;
    }

    return (unsigned int)env;
}

void initFrequencyWave(void) {
    int i;
    int value;

    /* Onda cuadrada suavizada: mas brillante que triangular, menos rota
       que una cuadrada dura. Se calcula una vez para ahorrar CPU. */
    for (i = 0; i < 256; i++) {
        if (i < 12) {
            value = -64 + ((i * 128) / 12);
        } else if (i < 116) {
            value = 64;
        } else if (i < 140) {
            value = 64 - (((i - 116) * 128) / 24);
        } else if (i < 244) {
            value = -64;
        } else {
            value = -64 + (((i - 244) * 128) / 12);
        }

        if (value < -64) {
            value = -64;
        } else if (value > 64) {
            value = 64;
        }

        freqWave[i] = (signed char)value;
    }
}

void advanceVoicePhase(VoiceState *voice) {
    /* Mantiene la fase dentro del ciclo de 16 bits. */
    voice->phase += voice->step;
    while (voice->phase >= PHASE_SCALE) {
        voice->phase -= PHASE_SCALE;
    }
}

int frequencyWaveFromPhase(unsigned long phase, unsigned int volume) {
    unsigned int pos;

    /* Usa tabla para que las notas altas no bajen de tono por exceso de CPU. */
    pos = (unsigned int)(phase >> 8);
    return ((int)freqWave[pos] * (int)volume) / 64;
}

int renderToneSample(MusicTrack *track, VoiceState *voice) {
    unsigned int volume;
    int sample;

    /* Los instrumentos tonales usan ondas baratas de calcular en Turbo C. */
    if (voice->currentFreq <= 0) {
        return 0;
    }

    if (track->instrument == INST_BASS) {
        volume = envelopeVolume(48U, voice, 0);
    } else if (track->instrument == INST_LEAD) {
        volume = envelopeVolume(38U, voice, 0);
    } else if (track->instrument == INST_FREQ) {
        volume = envelopeVolume(FREQ_VOLUME, voice, 0);
    } else {
        volume = envelopeVolume(42U, voice, 1);
    }

    advanceVoicePhase(voice);

    if (track->instrument == INST_BASS) {
        sample = (voice->phase < 24576UL) ? (int)volume : -((int)volume);
    } else if (track->instrument == INST_FREQ) {
        sample = frequencyWaveFromPhase(voice->phase, volume);
    } else {
        sample = (voice->phase < 32768UL) ? (int)volume : -((int)volume);
    }

    return sample;
}

int renderDrumSample(VoiceState *voice) {
    unsigned int env;
    unsigned long shortPart;
    int sample;

    /* La bateria se sintetiza con ruido y golpes cuadrados cortos. */
    if (voice->currentFreq == NOTE_MUTE) {
        return 0;
    }

    if (voice->currentFreq == DRUM_KICK) {
        env = envelopeVolume(72U, voice, 1);
        advanceVoicePhase(voice);
        return (voice->phase < 32768UL) ? (int)env : -((int)env);
    }

    voice->noise = (unsigned int)(voice->noise * 25173U + 13849U);

    if (voice->currentFreq == DRUM_HAT) {
        shortPart = voice->totalSamples / 5UL;
        if (shortPart < 1UL) {
            shortPart = 1UL;
        }
        if (voice->samplePos > shortPart) {
            return 0;
        }
        env = (unsigned int)((45UL * (shortPart - voice->samplePos)) /
                             shortPart);
        sample = (voice->noise & 1U) ? (int)env : -((int)env);
        return sample;
    }

    if (voice->currentFreq == DRUM_SNARE) {
        shortPart = voice->totalSamples / 2UL;
        if (shortPart < 1UL) {
            shortPart = 1UL;
        }
        if (voice->samplePos > shortPart) {
            return 0;
        }
        env = (unsigned int)((58UL * (shortPart - voice->samplePos)) /
                             shortPart);
        sample = (voice->noise & 0x80U) ? (int)env : -((int)env);
        return sample;
    }

    /* En Bateria tambien se aceptan notas normales como toms afinados. */
    if (voice->currentFreq > 0) {
        env = envelopeVolume(52U, voice, 1);
        advanceVoicePhase(voice);
        return (voice->phase < 32768UL) ? (int)env : -((int)env);
    }

    return 0;
}

int renderTrackSample(int trackIndex) {
    MusicTrack *track;
    VoiceState *voice;
    int sample;

    /* Genera una muestra firmada para una pista y avanza su contador. */
    track = &tracks[trackIndex];
    voice = &voices[trackIndex];

    if (!voice->active) {
        return 0;
    }

    if (track->instrument == INST_DRUM) {
        sample = renderDrumSample(voice);
    } else {
        sample = renderToneSample(track, voice);
    }

    if (voice->samplesLeft > 0UL) {
        voice->samplesLeft--;
        voice->samplePos++;
    }

    return sample;
}

int updateActiveEvents(void) {
    int i;
    int activeTracks;

    /* Inicia eventos pendientes antes de producir o saltar una muestra. */
    activeTracks = 0;
    for (i = 0; i < trackCount; i++) {
        if (voices[i].active && voices[i].samplesLeft == 0UL) {
            startNextEvent(i);
        }

        if (voices[i].active) {
            activeTracks++;
        }
    }

    return activeTracks;
}

void advanceTrackSilent(int trackIndex) {
    MusicTrack *track;
    VoiceState *voice;

    /* Avanza una muestra interna sin mandarla al DSP. Esto conserva tempo
       y afinacion cuando Direct DAC no alcanza el rate solicitado. */
    track = &tracks[trackIndex];
    voice = &voices[trackIndex];

    if (!voice->active) {
        return;
    }

    if (track->instrument == INST_DRUM) {
        if (voice->currentFreq == DRUM_KICK || voice->currentFreq > 0) {
            advanceVoicePhase(voice);
        }

        if (voice->currentFreq == DRUM_SNARE ||
            voice->currentFreq == DRUM_HAT) {
            voice->noise = (unsigned int)(voice->noise * 25173U + 13849U);
        }
    } else if (voice->currentFreq > 0) {
        advanceVoicePhase(voice);
    }

    if (voice->samplesLeft > 0UL) {
        voice->samplesLeft--;
        voice->samplePos++;
    }
}

int catchUpLateSamples(unsigned long *targetTicks, unsigned long *remainder) {
    long lagTicks;
    unsigned long missedLong;
    unsigned int missedSamples;
    unsigned int skipped;
    int i;

    /* Desactivado por defecto: con Direct DAC el salto de muestras puede
       empeorar el timbre. Es mejor usar un SAMPLE_RATE sostenible. */
    if (!ENABLE_CATCHUP) {
        return 1;
    }

    /* Si se activa, saltamos muestras internas cuando el render llega tarde. */
    if (sampleStep == 0UL) {
        return 1;
    }

    lagTicks = (long)(timerNow() - *targetTicks);
    if (lagTicks < (long)sampleStep) {
        return 1;
    }

    missedLong = ((unsigned long)lagTicks) / sampleStep;
    if (missedLong > (unsigned long)MAX_CATCHUP_SAMPLES) {
        missedSamples = MAX_CATCHUP_SAMPLES;
    } else {
        missedSamples = (unsigned int)missedLong;
    }

    for (skipped = 0; skipped < missedSamples; skipped++) {
        if (updateActiveEvents() == 0) {
            return 0;
        }

        for (i = 0; i < trackCount; i++) {
            advanceTrackSilent(i);
        }

        addSamplePeriod(targetTicks, remainder);
    }

    skippedSamplesTotal += (unsigned long)missedSamples;

    if ((long)(timerNow() - *targetTicks) >
        ((long)sampleStep * (long)MAX_CATCHUP_SAMPLES)) {
        *targetTicks = timerNow();
        *remainder = 0UL;
    }

    return 1;
}

int playSongOnce(void) {
    unsigned long targetTicks;
    unsigned long remainder;
    unsigned long sampleCounter;
    int i;
    int activeTracks;
    int mix;
    int loopActive;

    /* Mezcla todas las pistas a una salida unsigned 8-bit mono. */
    loopActive = songLoopEnabled || LOOP_SONG;
    resetVoicesFromStart();
    skippedSamplesTotal = 0UL;
    targetTicks = timerNow();
    remainder = 0UL;
    sampleCounter = 0UL;

    while (1) {
        activeTracks = updateActiveEvents();
        if (activeTracks == 0) {
            if (loopActive) {
                resetVoicesFromLoop();
                targetTicks = timerNow();
                remainder = 0UL;
                continue;
            }

            return 1;
        }

        if ((sampleCounter & 127UL) == 0UL && userAbortRequested()) {
            return 2;
        }

        waitUntil(targetTicks);

        mix = 128;
        for (i = 0; i < trackCount; i++) {
            mix += renderTrackSample(i);
        }

        if (mix < 0) {
            mix = 0;
        } else if (mix > 255) {
            mix = 255;
        }

        if (!sbDirectSample((unsigned char)mix)) {
            return 3;
        }

        addSamplePeriod(&targetTicks, &remainder);
        if (!catchUpLateSamples(&targetTicks, &remainder)) {
            return 1;
        }

        sampleCounter++;
    }
}

void playPcSpeakerFallback(void) {
    MusicTrack *track;
    MusicEvent *event;
    unsigned int i;
    unsigned int durationMs;

    /* Respaldo monofonico si Sound Blaster no responde. */
    if (trackCount <= 0) {
        return;
    }

    track = &tracks[0];
    for (i = 0; i < track->eventCount; i++) {
        event = &track->events[i];
        durationMs = (unsigned int)
            ((durationToSamples(event->durationDiv) * 1000UL) / SAMPLE_RATE);

        if (event->freqHz > 0) {
            sound((unsigned int)event->freqHz);
        } else {
            nosound();
        }

        delay(durationMs);
        nosound();

        if (userAbortRequested()) {
            return;
        }
    }
}

int main(int argc, char *argv[]) {
    int loadResult;
    int playResult;
    int checkMode;
    char *requestedPath;
    int argIndex;

    clrscr();
    prepareTiming();

    printf("VSBOX - Musica 8-bit escrita a mano\n\n");

    checkMode = 0;
    requestedPath = NULL;

    /* Argumentos simples: MUSIC8.EXE [archivo] [CHECK]. */
    for (argIndex = 1; argIndex < argc; argIndex++) {
        if (equalsIgnoreCase(argv[argIndex], "CHECK")) {
            checkMode = 1;
        } else {
            requestedPath = argv[argIndex];
        }
    }

    loadResult = loadMusicFile(requestedPath);
    if (loadResult == 0) {
        if (requestedPath != NULL) {
            printf("No se encontro: %s\n", requestedPath);
        } else {
            printf("No se encontro MUSIC.TXT.\n");
            printf("Rutas probadas:\n");
            printf("  MUSIC.TXT\n");
            printf("  C:\\TURBOC3\\MUSIC.TXT\n");
            printf("  C:\\PROJECTS\\SONIDO\\MUSICA8\\MUSIC.TXT\n");
        }
        return 1;
    }
    if (loadResult < 0) {
        printf("\nError leyendo el archivo de musica.\n");
        return 1;
    }

    printLoadedTracks();

    if (checkMode) {
        /* Modo de validacion: no espera tecla ni toca el DSP. */
        printf("\nCHECK: archivo cargado correctamente.\n");
        return 0;
    }

    printf("\nESC cancela la reproduccion.\n");
    printf("Presiona una tecla para iniciar...");
    getch();

    if (!sbReset()) {
        printf("\nNo se detecto Sound Blaster en 0x220.\n");
        printf("Usando PC speaker con la primera pista.\n");
        playPcSpeakerFallback();
        return 1;
    }

    sbSpeakerOn();

    playResult = playSongOnce();

    sbSpeakerOff();
    delay(150);

    if (skippedSamplesTotal > 0UL) {
        printf("\nMuestras saltadas para conservar tempo/pitch: %lu\n",
               skippedSamplesTotal);
    }

    if (playResult == 2) {
        printf("\nReproduccion cancelada.\n");
    } else if (playResult == 3) {
        printf("\nError escribiendo al DSP de Sound Blaster.\n");
    } else {
        printf("\nReproduccion finalizada.\n");
    }

    return 0;
}
