/* Pairbrush: original small DOS paint app for Tandy 1000 EX + WEGA. */
#ifdef HOST_UI
#include "HOSTDOS.H"
#else
#include <dos.h>
#include <conio.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifndef HOST_UI
#define MK_FP(s,o) ((void far *)(((unsigned long)(s) << 16) | (unsigned)(o)))
#endif
#include "CORE.H"
#ifdef HOST_UI
typedef unsigned short DOS_WORD;
#else
typedef unsigned DOS_WORD;
#endif
static BYTE far *video = (BYTE far *)0xb8000000UL;
static BYTE saved[32768];
static unsigned cursors[8], cursor_shape, old_mode, old_page, owned;
static unsigned image_segment[3];
/* 5x7 glyph columns: digits, A-Z, then punctuation used by labels. */
static const unsigned char font[][5] = {
 {62,81,73,69,62},{0,66,127,64,0},{66,97,81,73,70},
 {33,65,69,75,49},{24,20,18,127,16},{39,69,69,69,57},
 {60,74,73,73,48},{1,113,9,5,3},{54,73,73,73,54},
 {6,73,73,41,30},
 {126,17,17,17,126},{127,73,73,73,54},{62,65,65,65,34},
 {127,65,65,34,28},{127,73,73,73,65},{127,9,9,9,1},
 {62,65,73,73,122},{127,8,8,8,127},{0,65,127,65,0},
 {32,64,65,63,1},{127,8,20,34,65},{127,64,64,64,64},
 {127,2,12,2,127},{127,4,8,16,127},{62,65,65,65,62},
 {127,9,9,9,6},{62,65,81,33,94},{127,9,25,41,70},
 {38,73,73,73,50},{1,1,127,1,1},{63,64,64,64,63},
 {31,32,64,32,31},{63,64,56,64,63},{99,20,8,20,99},
 {7,8,112,8,7},{97,81,73,69,67},
 {0,8,8,8,0},{0,64,48,12,3},{0,34,20,8,0},
 {0,0,54,54,0},{0,127,65,65,0},{0,65,65,127,0},
 {20,20,20,20,20},{0,96,96,0,0},{64,64,64,64,64}
};
static void mode(unsigned n)
{
    union REGS r; r.x.ax = n; int86(0x10, &r, &r);
}
static void pixel(int x, int y, unsigned c)
{
    unsigned p; unsigned char v;
    if (x < 0 || x >= 320 || y < 0 || y >= 200 || c > 15) return;
    p = video_offset(x, y); v = video[p];
    video[p] = (unsigned char)((x & 1) ? (v & 240) | c :
                              (v & 15) | (c << 4));
}
static void text(int x, int y, const char *s)
{
    int col, row, n; char c;
    while ((c = *s++) != 0) {
        n = -1;
        if (c >= '0' && c <= '9') n = c - '0';
        if (c >= 'A' && c <= 'Z') n = c - 'A' + 10;
        if (c == '-') n = 36;
        if (c == '/') n = 37;
        if (c == '>') n = 38;
        if (c == ':') n = 39;
        if (c == '[') n = 40;
        if (c == ']') n = 41;
        if (c == '=') n = 42;
        if (c == '.') n = 43;
        if (c == '_') n = 44;
        if (n >= 0) for (col = 0; col < 5; col++)
            for (row = 0; row < 7; row++)
                if (font[n][col] & (1 << row)) pixel(x+col, y+row, 15);
        x += 6;
    }
}
static unsigned dos_get_strategy(unsigned *value)
{
    union REGS r; r.x.ax = 0x5800; int86(0x21, &r, &r);
    if (r.x.cflag) return r.x.ax;
    *value = r.x.ax; return 0;
}
static unsigned dos_set_strategy(unsigned value)
{
    union REGS r; r.x.ax = 0x5801; r.x.bx = value;
    int86(0x21, &r, &r); return r.x.cflag ? r.x.ax : 0;
}
static unsigned dos_allocate(unsigned paras, unsigned *segment)
{
    union REGS r; r.h.ah = 0x48; r.x.bx = paras;
    int86(0x21, &r, &r);
    if (r.x.cflag) return r.x.ax;
    *segment = r.x.ax; return 0;
}
static unsigned dos_free_block(unsigned segment)
{
    return _dos_freemem(segment);
}
/* Read-only post-cleanup arena check: the entire overlap must be free. */
static int dos_overlap_free(void)
{
    union REGS r; struct SREGS sr;
    unsigned seg, end, count; int covered = 0;
    unsigned char far *m;
    r.h.ah = 0x52; int86x(0x21, &r, &r, &sr);
    if (r.x.bx < 2) return 0;
    seg = *(DOS_WORD far *)MK_FP(sr.es, r.x.bx-2);
    for (count = 0; count < 4096; count++) {
        if (!seg || seg >= 0x9c00) return 0;
        m = (unsigned char far *)MK_FP(seg, 0);
        if (m[0] != 'M' && m[0] != 'Z') return 0;
        end = seg + 1 + *(DOS_WORD far *)(m+3);
        if (end <= seg || end > 0x9c00) return 0;
        if (seg+1 <= 0x9800 && end >= 0x9c00 &&
            *(DOS_WORD far *)(m+1) == 0) covered = 1;
        if (m[0] == 'Z') return covered && end == 0x9c00;
        seg = end;
    }
    return 0;
}
static int dos_valid_video_block(unsigned segment)
{
#ifdef HOST_QA
    return segment == 0x9800;
#else
    unsigned char far *m;
    if (segment != 0x9800) return 0;
    m = (unsigned char far *)MK_FP(segment-1, 0);
    return m[0] == 'Z' && *(DOS_WORD far *)(m+1) == _psp &&
           *(DOS_WORD far *)(m+3) == 0x400;
#endif
}
#include "RESMEM.H"
/* Reserve exposed Tandy video overlap for this process only. */
static int reserve_video(void)
{
    union REGS r; struct SREGS sr; unsigned seg, next, count;
    unsigned char far *m;
    memset(&r,0,sizeof(r));
#ifndef HOST_UI
    if (*(unsigned char far *)0xffff000eUL != 255 ||
        *(unsigned char far *)0xfc000000UL != 0x21 ||
        *(unsigned far *)0x00400013UL != 624) return 0;
#endif
    r.h.ah = 0x30; int86(0x21, &r, &r); if (r.h.al < 3) return 0;
    r.h.ah = 0x52; int86x(0x21, &r, &r, &sr);
    if (r.x.bx < 2) return 0;
    seg = *(DOS_WORD far *)MK_FP(sr.es, r.x.bx-2);
    for (count = 0; count < 4096; count++) {
        if (!seg || seg >= 0x9c00) return 0;
        m = (unsigned char far *)MK_FP(seg, 0);
        if (m[0] != 'M' && m[0] != 'Z') return 0;
        next = seg + 1 + *(DOS_WORD far *)(m+3);
        if (next <= seg || next > 0x9c00) return 0;
        if (m[0] == 'Z') break;
        seg = next;
    }
    if (count == 4096 || next != 0x9c00 || seg > 0x97ff ||
        *(DOS_WORD far *)(m+1) != 0) return 0;
    return reserve_block();
}
static int full_view, browser, view_x, view_y, pen_down, mouse_present;
static int cursor_x = 76, cursor_y = 68, pointer_x = 161, pointer_y = 92;
static int cursor_on, last_mx = -1, last_my = -1, last_buttons;
static int mouse_stroke, mouse_segment, stroke_x, stroke_y;
static int mouse_ignore_release;
static unsigned browser_original;
static const char *qa_keys;
static char message[54] = "P PAIRS   F FULL   H HELP";
static const char hex_digits[] = "0123456789ABCDEF";
static void mouse_call(unsigned n, union REGS *r)
{ r->x.ax = n; int86(0x33,r,r); }
static void mouse_start(int disabled)
{
    union REGS r; void (interrupt far *v)(void);
    mouse_present = 0;
    if (disabled) return;
    v = _dos_getvect(0x33);
    if (!v) return;
#ifndef HOST_UI
    if (*(BYTE far *)v == 0xcf) return;
#endif
    /* Reset leaves the native cursor hidden. Use our bank-aware pointer.
     * Never show a DOS driver cursor over application VRAM writes. */
    mouse_call(0,&r); if (r.x.ax != 0xffff) return;
    mouse_present = 1;
    r.x.cx = 0; r.x.dx = 639; mouse_call(7,&r);
    r.x.cx = 0; r.x.dx = 199; mouse_call(8,&r);
}
static void xor_pixel(int x, int y)
{
    if (x < 0 || x >= 320 || y < 0 || y >= 200) return;
    video[video_offset(x,y)] ^= (BYTE)((x & 1) ? 15 : 240);
}
static void cursor_toggle(void)
{
    int d;
    for (d = -3; d <= 3; d++) {
        xor_pixel(pointer_x+d,pointer_y);
        if (d) xor_pixel(pointer_x,pointer_y+d);
    }
    cursor_on ^= 1;
}
static void cursor_hide(void) { if (cursor_on) cursor_toggle(); }
static void cursor_show(void) { if (!cursor_on) cursor_toggle(); }
static void rect(int x, int y, int w, int h, unsigned pair)
{
    int xx, yy;
    /* Every filled swatch begins at absolute even x. */
    for (yy = y; yy < y+h; yy++)
        for (xx = x; xx < x+w; xx += 2)
            video[video_offset(xx,yy)] = (BYTE)pair;
}
static void box(int x, int y, int w, int h)
{
    int d;
    for (d = 0; d < w; d++) {
        pixel(x+d,y,15); pixel(x+d,y+h-1,15);
    }
    for (d = 0; d < h; d++) {
        pixel(x,y+d,15); pixel(x+w-1,y+d,15);
    }
}
static int canvas_width(void) { return full_view ? 160 : 152; }
static int canvas_height(void) { return full_view ? 200 : 136; }
static void canvas_draw(int l, int t, int r, int b)
{
    int x, y, sx, sy, ox, oy, vx, vy;
    if (browser) return;
    ox = full_view ? 0 : 8; oy = full_view ? 0 : 24;
    vx = full_view ? 0 : view_x; vy = full_view ? 0 : view_y;
    if (l < vx) l = vx;
    if (t < vy) t = vy;
    if (r >= vx+canvas_width()) r = vx+canvas_width()-1;
    if (b >= vy+canvas_height()) b = vy+canvas_height()-1;
    for (y = t; y <= b; y++) {
        sy = y-vy+oy;
        for (x = l; x <= r; x++) {
            sx = (x-vx)*2+ox;
            video[video_offset(sx,sy)] = document[y*DOC_W+x];
        }
    }
}
static void status_draw(void)
{
    char s[54];
    if (full_view || browser) return;
    rect(0,0,320,20,0x11);
    sprintf(s,"PAIRBRUSH  %c%c  SIZE %d  %s",
            hex_digits[color_pair >> 4],hex_digits[color_pair & 15],
            brush_size,modified ? "EDITED" : "READY");
    text(8,3,s);
    rect(274,12,36,8,color_pair);
    rect(0,160,320,26,0x11);
    text(8,162,"P PAIRS  F FULL  U UNDO  S SAVE  L LOAD");
    text(8,173,message);
}
static void browser_draw(void)
{
    int a, b; char s[54];
    rect(0,0,320,200,0x00);
    text(8,2,"PAIR LIBRARY - A EVEN / B ODD");
    sprintf(s,"SELECT %c%c   ENTER/P RETURN   ESC CANCEL",
            hex_digits[color_pair >> 4],hex_digits[color_pair & 15]);
    text(8,13,s);
    for (b = 0; b < 16; b++) {
        s[0] = hex_digits[b]; s[1] = 0; text(22+b*18,24,s);
    }
    for (a = 0; a < 16; a++) {
        s[0] = hex_digits[a]; s[1] = 0; text(4,35+a*9,s);
        for (b = 0; b < 16; b++) rect(20+b*18,34+a*9,16,8,a*16+b);
    }
    box(19+(color_pair & 15)*18,33+(color_pair >> 4)*9,18,10);
    text(8,182,"DIAGONAL AA = 16 NATIVE SOLIDS");
    text(8,193,"CHOOSE ON TV - NO RGB CALIBRATION");
}
static void screen_draw(void)
{
    int i;
    cursor_hide();
    if (browser) { browser_draw(); return; }
    if (!full_view) {
        rect(0,0,320,200,0x11); box(6,22,308,140);
        for (i = 0; i < 16; i++) rect(8+i*19+(i&1),188,16,10,i*17);
        /* The odd spacing above is rounded up to an even physical start. */
    }
    canvas_draw(0,0,159,199); status_draw();
}
static void keyboard_pointer(void)
{
    int changed = 0;
    cursor_hide();
    if (browser) {
        pointer_x = 28+(color_pair & 15)*18;
        pointer_y = 38+(color_pair >> 4)*9; return;
    }
    if (!full_view) {
        if (cursor_x < view_x) { view_x = cursor_x; changed = 1; }
        if (cursor_x >= view_x+152) { view_x = cursor_x-151; changed = 1; }
        if (cursor_y < view_y) { view_y = cursor_y; changed = 1; }
        if (cursor_y >= view_y+136) { view_y = cursor_y-135; changed = 1; }
        if (changed) screen_draw();
    }
    pointer_x = (cursor_x-(full_view ? 0 : view_x))*2+
                (full_view ? 1 : 9);
    pointer_y = cursor_y-(full_view ? 0 : view_y)+(full_view ? 0 : 24);
}
static void paint_dirty(void)
{
    cursor_hide();
    if (dirty_right >= dirty_left)
        canvas_draw(dirty_left,dirty_top,dirty_right,dirty_bottom);
    dirty_reset();
}
static int read_key(void)
{
    int k;
    if (qa_keys && *qa_keys) return (unsigned char)*qa_keys++;
    k = getch(); if (!k || k == 224) k = 256+getch(); return k;
}
static int confirm(const char *s)
{
    int key;
    cursor_hide(); rect(16,76,288,44,0x00); box(16,76,288,44);
    text(26,86,s); text(26,103,"Y YES   ANY OTHER KEY CANCEL");
    key = read_key(); screen_draw(); return key == 'y' || key == 'Y';
}
static void file_dialog(int loading)
{
    char name[13], s[54]; int k, n = 0, result;
    if (loading && modified && !confirm("LOAD REPLACES ART - CONTINUE?"))
        return;
    cursor_hide();
    for (;;) {
        rect(16,66,288,68,0x00); box(16,66,288,68);
        text(26,76,loading ? "LOAD NAME - 1 TO 8 LETTERS" :
                                   "SAVE NEW NAME - 1 TO 8 LETTERS");
        name[n] = 0; sprintf(s,"%s.TDP",name); text(26,94,s);
        text(26,115,"ENTER OK   ESC CANCEL   BACKSPACE");
        k = read_key();
        if (k == 27) { screen_draw(); return; }
        if (k == 13 && n) break;
        if (k == 8 && n) n--;
        if (k >= 'a' && k <= 'z') k -= 32;
        if (n < 8 && ((k >= 'A' && k <= 'Z') ||
            (k >= '0' && k <= '9') || k == '_' || k == '-'))
            name[n++] = (char)k;
    }
    strcpy(name+n,".TDP");
    result = loading ? load_document(name) : save_document(name);
    if (result == 1) sprintf(message,"%s %s",loading ? "LOADED" : "SAVED",name);
    else if (result == 2) strcpy(message,"NAME EXISTS - SAVE WITH A NEW NAME");
    else strcpy(message,loading ? "LOAD FAILED - ART KEPT" :
                                "SAVE FAILED - CHECK DISK");
    screen_draw();
}
static void help(void)
{
    cursor_hide(); rect(0,0,320,200,0x00);
    text(8,8,"PAIRBRUSH - PAINT ON YOUR TV");
    text(8,28,"LEFT MOUSE DRAG PAINTS - RIGHT PICKS");
    text(8,42,"ARROWS MOVE - SPACE STAMPS - D PEN");
    text(8,56,"[ ] BRUSH 1 2 4 8 CELLS - U UNDO/REDO");
    text(8,70,"P ALL 256 PAIRS - ARROWS THEN ENTER");
    text(8,84,"BOTTOM STRIP SELECTS NATIVE SOLIDS");
    text(8,98,"F FULL CANVAS - ARROWS PAN SMALL VIEW");
    text(8,112,"C CLEAR - S SAVE - L LOAD - ESC QUIT");
    text(8,126,"FILES: NAME.TDP - EXISTING FILES KEPT");
    text(8,146,"A ON EVEN PIXELS / B ON ODD PIXELS");
    text(8,160,"OPPOSITE ORDERS ARE SEPARATE COLORS");
    text(8,174,"NO TV COLOR CALIBRATION IS CLAIMED");
    text(8,190,"PRESS ANY KEY TO RETURN");
    read_key(); screen_draw();
}
static int command(int key)
{
    int x = cursor_x, y = cursor_y;
    if (key >= 'a' && key <= 'z') key -= 32;
    if (browser) {
        if (key == 27 || key == 13 || key == 'P') {
            if (key == 27) color_pair = browser_original;
            browser = 0; screen_draw(); keyboard_pointer(); return 1;
        }
        if (key == 331) color_pair = (color_pair & 240) |
                                    ((color_pair-1) & 15);
        if (key == 333) color_pair = (color_pair & 240) |
                                    ((color_pair+1) & 15);
        if (key == 328) color_pair = (color_pair-16) & 255;
        if (key == 336) color_pair = (color_pair+16) & 255;
        screen_draw(); keyboard_pointer(); return 1;
    }
    if (key == 27) {
        pen_down = 0;
        if (!modified || confirm("QUIT WITHOUT SAVING?")) return 0;
        return 1;
    }
    if (key == 331 && x > 0) x--;
    if (key == 333 && x < 159) x++;
    if (key == 328 && y > 0) y--;
    if (key == 336 && y < 199) y++;
    if (x != cursor_x || y != cursor_y) {
        if (pen_down) { stroke(cursor_x,cursor_y,x,y); paint_dirty(); }
        cursor_x = x; cursor_y = y; keyboard_pointer(); return 1;
    }
    if (key == 'D') {
        pen_down ^= 1;
        if (pen_down) { snapshot(); stamp(x,y); paint_dirty(); }
        strcpy(message,pen_down ? "PEN DOWN - D TO LIFT" : "PEN UP");
    } else if (key == ' ') {
        snapshot(); stamp(x,y); paint_dirty();
    } else {
        pen_down = 0; mouse_stroke = 0;
        if (key == 'P') {
            browser_original = color_pair; browser = 1;
            screen_draw(); keyboard_pointer();
        }
        if (key == 'F') {
            full_view ^= 1; screen_draw(); keyboard_pointer();
        }
        if (key == 'U') { undo_swap(); paint_dirty(); }
        if (key == 'C' && confirm("CLEAR CANVAS TO BLACK?")) {
            clear_document(); paint_dirty();
        }
        if (key == 'S' || key == 'L') file_dialog(key == 'L');
        if (key == 'H' || key == '?') help();
        if (key == '[' && brush_size > 1) brush_size /= 2;
        if (key == ']' && brush_size < 8) brush_size *= 2;
    }
    status_draw(); return 1;
}
static void mouse_input(int mx, int my, int buttons)
{
    int x, y, fresh, inside, i;
    fresh = (buttons & 1) && !(last_buttons & 1);
    cursor_hide(); pointer_x = mx; pointer_y = my;
    if (mouse_ignore_release) {
        if (buttons & 1) { last_buttons = buttons; return; }
        mouse_ignore_release = 0;
    }
    if (browser) {
        if (fresh && mx >= 20 && mx < 308 && my >= 34 && my < 178) {
            color_pair = (unsigned)(((my-34)/9)*16+(mx-20)/18);
            if (color_pair <= 255) {
                browser = 0; mouse_ignore_release = 1; screen_draw();
            }
        }
    } else {
        x = full_view ? mx/2 : (mx-8)/2+view_x;
        y = full_view ? my : my-24+view_y;
        inside = full_view || (mx >= 8 && mx < 312 && my >= 24 && my < 160);
        if (inside && (buttons & 2) && !(last_buttons & 2)) {
            color_pair = document[y*DOC_W+x]; status_draw();
        }
        if (inside && (buttons & 1)) {
            pen_down = 0;
            if (!mouse_stroke) {
                snapshot(); stamp(x,y); mouse_stroke = 1;
            } else if (mouse_segment) stroke(stroke_x,stroke_y,x,y);
            else stamp(x,y);
            mouse_segment = 1;
            stroke_x = x; stroke_y = y;
            cursor_x = x; cursor_y = y; paint_dirty();
        } else {
            mouse_segment = 0;
            if (!(buttons & 1)) mouse_stroke = 0;
        }
        if (fresh && !inside && !full_view) {
            mouse_ignore_release = 1;
            if (my < 20 && mx >= 272) command('P');
            else if (my < 20 && mx >= 128 && mx < 230) {
                brush_size = brush_size == 8 ? 1 : brush_size*2;
                status_draw();
            } else if (my >= 188 && mx >= 8 && mx < 310) {
                i = (mx-8)/19; if (i < 16) color_pair = (unsigned)i*17;
                status_draw();
            } else if (my >= 160 && my < 172) {
                if (mx < 55) command('P');
                else if (mx < 109) command('F');
                else if (mx < 163) command('U');
                else if (mx < 217) command('S');
                else command('L');
            }
        }
    }
    if (!(buttons & 1) && (last_buttons & 1)) status_draw();
    last_buttons = buttons;
}
static void mouse_report(unsigned virtual_x, unsigned screen_y,
                         unsigned buttons)
{
    unsigned screen_x;
    screen_x = virtual_x/2;
    if (screen_x > 319) screen_x = 319;
    if (screen_y > 199) screen_y = 199;
    buttons &= 3;
    /* Only bounded values are converted to the signed UI coordinate type. */
    if ((int)screen_x != last_mx || (int)screen_y != last_my ||
        (int)buttons != last_buttons) {
        last_mx = (int)screen_x; last_my = (int)screen_y;
        mouse_input(last_mx,last_my,(int)buttons);
    }
}
static void restore(void)
{
    union REGS r; unsigned i, error;
    if (owned) {
        cursor_hide();
        if (mouse_present) mouse_call(0,&r);
        mode(old_mode);
        for (i = 0; i < 32768U; i++) video[i] = saved[i];
        for (i = 0; i < 8; i++) {
            r.h.ah = 2; r.h.bh = (BYTE)i;
            r.x.dx = cursors[i]; int86(0x10,&r,&r);
        }
        r.h.ah = 1; r.x.cx = cursor_shape; int86(0x10,&r,&r);
        r.h.ah = 5; r.h.al = (BYTE)old_page; int86(0x10,&r,&r);
        if (mouse_present) mouse_call(0,&r);
        owned = 0;
    }
    release_reserved(); restore_strategy();
    for (i = 0; i < 3; i++) if (image_segment[i]) {
        error = _dos_freemem(image_segment[i]);
        if (!error) image_segment[i] = 0;
        else { cleanup_error = 1; last_dos_error = error; }
    }
}
static int dump(const char *name)
{
    FILE *f; unsigned i; int bad = 0;
    cursor_hide(); f = fopen(name,"wb"); if (!f) return 0;
    for (i = 0; i < 32768U; i++)
        if (fputc(video[i],f) == EOF) { bad = 1; break; }
    if (fclose(f)) bad = 1;
#ifdef CAPTURE_QA
    if (!capture_screen(name)) bad = 1;
#endif
    return !bad;
}
static int qa(void)
{
    unsigned i, strategy, sum, query; int bad = 0, guard = 1;
    BYTE header[16]; BYTE far *high; FILE *f; union REGS r;
    high = (BYTE far *)MK_FP(0x97ff,0);
    for (i = 0; i < 16; i++) header[i] = high[i];
    screen_draw(); if (!dump("START.RAW")) bad++;
    command('P'); if (!dump("PAIRS.RAW")) bad++; command(13);
    color_pair = 0x14; brush_size = 8;
    mouse_input(40,44,1); mouse_input(104,85,1); mouse_input(104,85,0);
    cursor_show(); cursor_hide();
    sum = image_crc(document); command('U');
    if (image_crc(document) == sum) bad++;
    command('U'); if (image_crc(document) != sum) bad++;
    qa_keys = "N"; command('C'); if (image_crc(document) != sum) bad++;
    qa_keys = "Y"; command('C'); command('U');
    if (image_crc(document) != sum) bad++;
    qa_keys = "N"; if (!command(27)) bad++;
    qa_keys = "\033"; command('S');
    qa_keys = "QAUI\r"; command('S');
    if (modified) bad++;
    color_pair = 0x6e; stamp(20,20); paint_dirty();
    qa_keys = "YQAUI\r"; command('L');
    if (modified || image_crc(document) != sum) bad++;
    remove("QAUI.TDP");
    command('F'); if (!dump("FULL.RAW")) bad++;
    command('F'); command('D'); command(333); command(336); command('D');
    if (!dump("PAINTED.RAW")) bad++;
    if (!dos_valid_video_block(reservation)) bad++;
    for (i = 0; i < 16; i++) if (header[i] != high[i]) guard = 0;
    qa_keys = "Y"; if (command(27)) bad++;
    restore(); query = dos_get_strategy(&strategy);
    r.h.ah = 15; int86(0x10,&r,&r);
    if ((unsigned)r.h.al != old_mode || (unsigned)r.h.bh != old_page) bad++;
    for (i = 0; i < 32768U; i++) if (video[i] != saved[i]) bad++;
    for (i = 0; i < 8; i++) {
        r.h.ah = 3; r.h.bh = (BYTE)i; int86(0x10,&r,&r);
        if (r.x.dx != cursors[i] || r.x.cx != cursor_shape) bad++;
    }
    if (!guard || !dos_overlap_free() || query || strategy != saved_strategy ||
        reservation || image_segment[0] || image_segment[1] ||
        image_segment[2] || strategy_pending || cleanup_error) bad++;
    f = fopen("SMOKE.TXT","w"); if (!f) return 1;
#ifdef HOST_UI
    fprintf(f,"HOST ADAPTER UI/scripted input/restoration: %s\n",
#else
    fprintf(f,"DOS UI/scripted painting/undo/Escape/restoration: %s\n",
#endif
            bad ? "FAIL" : "PASS");
    fprintf(f,"mouse_driver=%d guard16=%d strategy=%u cleanup=%u\n",
            mouse_present,guard,strategy,cleanup_error);
    fclose(f); return bad ? 1 : 0;
}
int main(int argc, char **argv)
{
    union REGS r; unsigned i; int running = 1, test = 0, disabled = 0;
    for (i = 1; i < (unsigned)argc; i++) {
        if (!strcmp(argv[i],"/QA")) test = 1;
        else if (!strcmp(argv[i],"/NOMOUSE")) disabled = 1;
        else {
            puts("PAIRBRUSH [/NOMOUSE] - Tandy 640K plain DOS 3+");
            puts("Mouse drag paints; arrows/space/D keyboard; H help; ESC exit.");
            return 0;
        }
    }
    if (atexit(restore)) { puts("Cannot register cleanup."); return 2; }
    r.h.ah = 15; int86(0x10,&r,&r); old_mode = r.h.al; old_page = r.h.bh;
    if (old_mode != 3 || !reserve_video()) {
        restore(); puts("Needs plain Tandy DOS 3+, 640K (BIOS 624K), mode 3,");
        puts("and free video overlap. Exit Windows and reboot to plain DOS.");
        return 2;
    }
    for (i = 0; i < 3; i++)
        if (_dos_allocmem(2000,&image_segment[i])) {
            image_segment[i] = 0; restore(); puts("Not enough free RAM.");
            return 3;
        }
    document = (BYTE far *)MK_FP(image_segment[0],0);
    undo_image = (BYTE far *)MK_FP(image_segment[1],0);
    load_image = (BYTE far *)MK_FP(image_segment[2],0);
    for (i = 0; i < DOC_N; i++) document[i] = 0;
    for (i = 0; i < 32768U; i++) saved[i] = video[i];
    for (i = 0; i < 8; i++) {
        r.h.ah = 3; r.h.bh = (BYTE)i; int86(0x10,&r,&r);
        cursors[i] = r.x.dx; cursor_shape = r.x.cx;
    }
    mode(9); owned = 1; r.h.ah = 15; int86(0x10,&r,&r);
    if (r.h.al != 9) { restore(); return 4; }
    mouse_start(disabled); dirty_reset();
    if (test) return qa();
    screen_draw(); keyboard_pointer();
    while (running) {
        cursor_show();
        if (kbhit()) { cursor_hide(); running = command(read_key()); }
        if (running && mouse_present) {
            mouse_call(3,&r);
            mouse_report(r.x.cx,r.x.dx,r.x.bx);
        }
    }
    restore();
    if (cleanup_error) {
        printf("DOS cleanup error %u; reboot before reuse.\n",last_dos_error);
        return 5;
    }
    return 0;
}
