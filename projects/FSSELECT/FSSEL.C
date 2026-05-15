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
#define PANEL_HEIGHT 16
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

/* Dibuja una interfaz mas clara y visual, sin cambiar la logica del selector. */
void draw_menu(int saved_mode, int selected_mode)
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

    set_style(YELLOW, BLUE);
    gotoxy(PANEL_LEFT + 5, PANEL_TOP + 14);
    cprintf("Seleccione una opcion: ");
}

/* Lee el teclado y permite mover la seleccion con flecha arriba/abajo.
   En DOS, las flechas llegan como dos codigos: 0/224 y luego el scan code. */
int select_mode_with_keyboard(int saved_mode)
{
    int selected_mode;
    int key;
    int scan_code;

    selected_mode = saved_mode;
    draw_menu(saved_mode, selected_mode);

    while (1) {
        key = getch();

        if (key == KEY_EXTENDED || key == KEY_EXTENDED_ALT) {
            scan_code = getch();

            if (scan_code == KEY_ARROW_UP) {
                selected_mode = MODE_FULLSCREEN;
                draw_menu(saved_mode, selected_mode);
            } else if (scan_code == KEY_ARROW_DOWN) {
                selected_mode = MODE_WINDOW;
                draw_menu(saved_mode, selected_mode);
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
}

int main(void)
{
    int mode;

    mode = read_mode();
    mode = select_mode_with_keyboard(mode);

    save_mode(mode);

    cprintf("\r\n\r\nModo seleccionado: %s\r\n",
            mode ? "PANTALLA COMPLETA" : "VENTANA");
    cprintf("Continuando...\r\n");

    /* Pausa breve para que el usuario alcance a ver la confirmacion. */
    delay(450);

    return 0;
}
