/* Host-only deterministic adapter; never linked into PAINT.EXE.
 * Reconstruction evidence, not emulation of Tandy hardware. */
#define far
#define interrupt
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
union REGS {
    struct { unsigned short ax,bx,cx,dx,si,di,cflag; } x;
    struct { unsigned char al,ah,bl,bh,cl,ch,dl,dh; } h;
};
struct SREGS { unsigned short es,cs,ss,ds; };
static unsigned char host_memory[1048576];
static unsigned host_mode = 3, host_page, host_strategy = 1;
static unsigned host_positions[8], host_shape = 0x0607;
static unsigned host_blocks[3], host_errors, host_mouse;
static unsigned _psp = 0x1000;
static void *host_pointer(unsigned seg, unsigned off)
{
    if (seg == 0xb800) seg = 0x9800;
    return host_memory+seg*16UL+off;
}
#define MK_FP(s,o) host_pointer((s),(o))
static void host_word(unsigned char *p, unsigned n)
{ p[0] = (unsigned char)n; p[1] = (unsigned char)(n >> 8); }
static void host_arena(int occupied)
{
    unsigned char *p = host_memory+0x90000;
    memset(p,0,16); p[0] = occupied ? 'M' : 'Z';
    host_word(p+3,occupied ? 0x07fe : 0x0bff);
    if (occupied) {
        p = host_memory+0x97ff0; memset(p,0,16); p[0] = 'Z';
        host_word(p+1,_psp); host_word(p+3,0x400);
    }
}
static void host_init(void)
{
    unsigned i;
    memset(host_memory,0,sizeof(host_memory)); host_arena(0);
    host_word(host_memory+0x1000,0x9000);
    for (i = 0; i < 32768U; i++) host_memory[0x98000+i] = (unsigned char)i;
}
static unsigned _dos_allocmem(unsigned n, unsigned *seg)
{
    unsigned i;
    if (n != 2000) return 8;
    for (i = 0; i < 3; i++) if (!host_blocks[i]) {
        *seg = 0x2000+i*0x0800; host_blocks[i] = *seg; return 0;
    }
    return 8;
}
static unsigned _dos_freemem(unsigned seg)
{
    unsigned i;
    if (seg == 0x9800) { host_arena(0); return 0; }
    for (i = 0; i < 3; i++) if (host_blocks[i] == seg) {
        host_blocks[i] = 0; return 0;
    }
    host_errors++; return 9;
}
static void int86(int n, union REGS *a, union REGS *b)
{
    unsigned ah = a->h.ah;
    *b = *a;
    if (n == 0x10) {
        if (ah == 0) { host_mode = a->h.al; memset(host_memory+0x98000,0,32768); }
        else if (ah == 15) { b->h.al = (unsigned char)host_mode; b->h.bh = (unsigned char)host_page; }
        else if (ah == 3) { b->x.dx = (unsigned short)host_positions[a->h.bh]; b->x.cx = (unsigned short)host_shape; }
        else if (ah == 2) host_positions[a->h.bh] = a->x.dx;
        else if (ah == 1) host_shape = a->x.cx;
        else if (ah == 5) host_page = a->h.al;
    } else if (n == 0x21) {
        b->x.cflag = 0;
        if (a->x.ax == 0x5800) b->x.ax = (unsigned short)host_strategy;
        else if (a->x.ax == 0x5801) host_strategy = a->x.bx;
        else if (ah == 0x48 && a->x.bx == 0x400) {
            host_arena(1); b->x.ax = 0x9800;
        } else if (ah == 0x30) b->h.al = 3;
    } else if (n == 0x33) {
        if (!a->x.ax) b->x.ax = host_mouse ? 0xffff : 0;
        if (a->x.ax == 1) host_errors++;
    }
}
static void int86x(int n, union REGS *a, union REGS *b, struct SREGS *s)
{
    *b = *a;
    if (n == 0x21 && a->h.ah == 0x52) { s->es = 0x100; b->x.bx = 2; }
}
static void (*_dos_getvect(unsigned n))(void)
{ (void)n; return NULL; }
static int getch(void) { host_errors++; return 27; }
static int kbhit(void) { return 1; }
