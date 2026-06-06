/* Selector de modo de pantalla para VSBOX/DOSBox.
   Este programa se ejecuta antes del build real y guarda la preferencia
   en C:\TURBOC3\FULLSCR.CFG para que el script host abra DOSBox con
   o sin el parametro -fullscreen. */

#include <conio.h>
#include <dos.h>
#include <stdio.h>
#include <string.h>

#define CFG_PATH "C:\\TURBOC3\\FULLSCR.CFG"
#define PANEL_LEFT 16
#define PANEL_TOP 5
#define PANEL_WIDTH 49
#define PANEL_HEIGHT 18
#define SELECT_TIMEOUT_SECONDS 5
#define SELECT_POLL_MS 100
#define SELECT_TICKS_PER_SECOND (1000 / SELECT_POLL_MS)
#define KEY_ESC 27
#define KEY_ENTER 13
#define KEY_EXTENDED 0
#define KEY_EXTENDED_ALT 224
#define KEY_ARROW_UP 72
#define KEY_ARROW_DOWN 80
#define MODE_WINDOW 0
#define MODE_FULLSCREEN 1

/* Lee la preferencia guardada. Si no existe, mantiene pantalla completa
   como valor inicial para conservar el comportamiento previo del proyecto. */
int read_mode(void)
{
    FILE *cfg;
    int ch;

    cfg = fopen(CFG_PATH, "r");
    if (cfg == NULL) {
        return 1;
    }

    ch = fgetc(cfg);
    fclose(cfg);

    if (ch == '0') {
        return 0;
    }

    return 1;
}

/* Guarda 1 para pantalla completa o 0 para ventana. */
void save_mode(int mode)
{
    FILE *cfg;

    cfg = fopen(CFG_PATH, "w");
    if (cfg == NULL) {
        cprintf("No se pudo guardar %s\r\n", CFG_PATH);
        return;
    }

    fprintf(cfg, "%d\n", mode ? 1 : 0);
    fclose(cfg);
}

/* Aplica colores de texto y fondo para mantener el codigo de dibujo legible. */
void set_style(int foreground, int background)
{
    textcolor(foreground);
    textbackground(background);
}

/* Limpia toda la pantalla con un color de fondo uniforme. */
void clear_screen(int background)
{
    textbackground(background);
    clrscr();
}

/* Dibuja una caja doble usando codigos de la pagina DOS 437. */
void draw_box(int left, int top, int width, int height, int foreground,
              int background)
{
    int i;
    int right;
    int bottom;

    right = left + width - 1;
    bottom = top + height - 1;

    set_style(foreground, background);

    gotoxy(left, top);
    cprintf("%c", (char)201);
    for (i = 0; i < width - 2; i++) {
        cprintf("%c", (char)205);
    }
    cprintf("%c", (char)187);

    for (i = top + 1; i < bottom; i++) {
        gotoxy(left, i);
        cprintf("%c", (char)186);
        gotoxy(right, i);
        cprintf("%c", (char)186);
    }

    gotoxy(left, bottom);
    cprintf("%c", (char)200);
    for (i = 0; i < width - 2; i++) {
        cprintf("%c", (char)205);
    }
    cprintf("%c", (char)188);
}

/* Escribe un texto centrado dentro de un ancho fijo. */
void print_centered(int row, int left, int width, char *text, int foreground,
                    int background)
{
    int col;

    col = left + ((width - (int)strlen(text)) / 2);
    if (col < left) {
        col = left;
    }

    set_style(foreground, background);
    gotoxy(col, row);
    cprintf("%s", text);
}

/* Dibuja una opcion del menu y resalta la seleccion navegable. */
void draw_option(int row, char *hotkey, char *label, int selected)
{
    int i;
    int left;

    left = PANEL_LEFT + 5;

    if (selected) {
        set_style(BLACK, CYAN);
    } else {
        set_style(WHITE, BLUE);
    }

    gotoxy(left, row);
    for (i = 0; i < PANEL_WIDTH - 10; i++) {
        cprintf(" ");
    }

    gotoxy(left + 2, row);
    cprintf("%s", hotkey);

    if (selected) {
        set_style(YELLOW, CYAN);
    } else {
        set_style(LIGHTCYAN, BLUE);
    }

    cprintf("  %s", label);
}

/* Limpia una fila interna del panel antes de reescribir texto variable. */
void clear_panel_line(int row)
{
    int i;

    set_style(LIGHTGRAY, BLUE);
    gotoxy(PANEL_LEFT + 3, row);
    for (i = 0; i < PANEL_WIDTH - 6; i++) {
        cprintf(" ");
    }
}

/* Muestra la cuenta regresiva que evita dejar el build detenido sin usuario. */
void draw_timeout_status(int saved_mode, int remaining_seconds, int timeout_active)
{
    char *mode_text;

    mode_text = saved_mode ? "PANTALLA COMPLETA" : "VENTANA";

    clear_panel_line(PANEL_TOP + 13);
    set_style(YELLOW, BLUE);
    gotoxy(PANEL_LEFT + 5, PANEL_TOP + 13);
    if (timeout_active) {
        cprintf("Auto en %d s: %s", remaining_seconds, mode_text);
    } else {
        cprintf("Auto pausado: confirma con ENTER o ESC.");
    }
}

/* Dibuja una interfaz mas clara y visual, sin cambiar la logica del selector. */
void draw_menu(int saved_mode, int selected_mode, int remaining_seconds,
               int timeout_active)
{
    clear_screen(BLUE);
    draw_box(PANEL_LEFT, PANEL_TOP, PANEL_WIDTH, PANEL_HEIGHT, WHITE, BLUE);

    print_centered(PANEL_TOP + 1, PANEL_LEFT, PANEL_WIDTH,
                   "VSBOX", YELLOW, BLUE);
    print_centered(PANEL_TOP + 2, PANEL_LEFT, PANEL_WIDTH,
                   "Selector de pantalla", WHITE, BLUE);

    set_style(LIGHTGRAY, BLUE);
    gotoxy(PANEL_LEFT + 5, PANEL_TOP + 4);
    cprintf("Modo guardado:");

    if (saved_mode) {
        set_style(BLACK, GREEN);
        cprintf(" PANTALLA COMPLETA ");
    } else {
        set_style(BLACK, LIGHTGRAY);
        cprintf(" VENTANA ");
    }

    draw_option(PANEL_TOP + 7, "[1]", "Ejecutar en pantalla completa",
                selected_mode == MODE_FULLSCREEN);
    draw_option(PANEL_TOP + 9, "[2]", "Ejecutar en ventana",
                selected_mode == MODE_WINDOW);

    set_style(LIGHTGRAY, BLUE);
    gotoxy(PANEL_LEFT + 5, PANEL_TOP + 12);
    cprintf("Usa flechas, 1/2, ENTER o ESC.");

    draw_timeout_status(saved_mode, remaining_seconds, timeout_active);

    set_style(YELLOW, BLUE);
    gotoxy(PANEL_LEFT + 5, PANEL_TOP + 15);
    cprintf("Seleccion: %s",
            selected_mode ? "PANTALLA COMPLETA" : "VENTANA");

    set_style(WHITE, BLUE);
    gotoxy(PANEL_LEFT + 5, PANEL_TOP + 16);
    cprintf("ENTER confirma, ESC conserva guardado.");
}

/* Lee el teclado y permite mover la seleccion con flecha arriba/abajo.
   En DOS, las flechas llegan como dos codigos: 0/224 y luego el scan code. */
int select_mode_with_keyboard(int saved_mode)
{
    int selected_mode;
    int key;
    int scan_code;
    int remaining_seconds;
    int last_remaining_seconds;
    int remaining_ticks;
    int timeout_active;

    selected_mode = saved_mode;
    timeout_active = 1;
    remaining_seconds = SELECT_TIMEOUT_SECONDS;
    last_remaining_seconds = remaining_seconds;
    remaining_ticks = SELECT_TIMEOUT_SECONDS * SELECT_TICKS_PER_SECOND;
    draw_menu(saved_mode, selected_mode, remaining_seconds, timeout_active);

    while (1) {
        if (kbhit()) {
            key = getch();
            if (timeout_active) {
                timeout_active = 0;
                draw_menu(saved_mode, selected_mode, remaining_seconds,
                          timeout_active);
            }

            if (key == KEY_EXTENDED || key == KEY_EXTENDED_ALT) {
                scan_code = getch();

                if (scan_code == KEY_ARROW_UP) {
                    selected_mode = MODE_FULLSCREEN;
                    draw_menu(saved_mode, selected_mode, remaining_seconds,
                              timeout_active);
                } else if (scan_code == KEY_ARROW_DOWN) {
                    selected_mode = MODE_WINDOW;
                    draw_menu(saved_mode, selected_mode, remaining_seconds,
                              timeout_active);
                }
            } else if (key == '1') {
                return MODE_FULLSCREEN;
            } else if (key == '2') {
                return MODE_WINDOW;
            } else if (key == KEY_ENTER) {
                return selected_mode;
            } else if (key == KEY_ESC) {
                return saved_mode;
            }
        }

        if (!timeout_active) {
            delay(SELECT_POLL_MS);
            continue;
        }

        /* No bloquea: revisa teclado cada 100 ms y actualiza solo el contador. */
        delay(SELECT_POLL_MS);
        remaining_ticks--;
        remaining_seconds = (remaining_ticks + SELECT_TICKS_PER_SECOND - 1) /
                            SELECT_TICKS_PER_SECOND;

        if (remaining_ticks <= 0) {
            return saved_mode;
        }

        if (remaining_seconds != last_remaining_seconds) {
            draw_timeout_status(saved_mode, remaining_seconds, timeout_active);
            last_remaining_seconds = remaining_seconds;
        }
    }
}

/* Muestra la confirmacion fuera del recuadro para no pisar el menu. */
void draw_final_status(int mode)
{
    int row;
    int i;

    row = PANEL_TOP + PANEL_HEIGHT + 1;
    set_style(LIGHTGRAY, BLACK);

    for (i = 0; i < 2; i++) {
        gotoxy(PANEL_LEFT, row + i);
        cprintf("                                                     ");
    }

    gotoxy(PANEL_LEFT, row);
    cprintf("Modo seleccionado: %s",
            mode ? "PANTALLA COMPLETA" : "VENTANA");
    gotoxy(PANEL_LEFT, row + 1);
    cprintf("Continuando...");
}

int main(void)
{
    int mode;

    mode = read_mode();
    mode = select_mode_with_keyboard(mode);

    save_mode(mode);

    draw_final_status(mode);

    /* Pausa breve para que el usuario alcance a ver la confirmacion. */
    delay(450);

    return 0;
}
