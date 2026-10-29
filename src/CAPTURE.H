/* Test-only DOSBox-X renderer screenshot adapter, not a shipping dependency.
 * Interface checked against official 2025.12.01 iglib.h / bios.cpp.
 * Original implementation uses 8-bit I/O only, valid for the 8088.
 * No desktop input injection or host window capture is used. */
#include <dos.h>
#include <conio.h>
#include <stdio.h>
static void capture_select(unsigned long index)
{
    int i;
    outp(0x2a,0);
    for (i = 0; i < 4; i++) {
        outp(0x28,(int)(index & 255UL)); index >>= 8;
    }
}
static unsigned long capture_read(void)
{
    unsigned long value = 0; int i;
    for (i = 0; i < 4; i++)
        value |= (unsigned long)(inp(0x29) & 255) << (i*8);
    return value;
}
static void capture_write(unsigned long value)
{
    int i;
    for (i = 0; i < 4; i++) {
        outp(0x29,(int)(value & 255UL)); value >>= 8;
    }
}
static unsigned long capture_ticks(void)
{
    union REGS r;
    r.x.ax = 0; int86(0x1a,&r,&r);
    return ((unsigned long)r.x.cx << 16) | r.x.dx;
}
static int capture_screen(const char *name)
{
    FILE *f; unsigned long start, status; int good = 1;
    capture_select(0);
    if (capture_read() != 0xd05b0740UL) good = 0;
    if (good) {
        /* Let a complete renderer frame settle after UI changes. */
        start = capture_ticks();
        while (capture_ticks()-start < 4UL) {}
        capture_select(0x00c54010UL); status = capture_read();
        if (status & 0xc0000000UL) good = 0;
        else {
            capture_write(1); start = capture_ticks();
            do {
                capture_select(0x00c54010UL); status = capture_read();
                if (!(status & 1UL)) break;
            } while (capture_ticks()-start < 36UL);
            if ((status & 1UL) || (inp(0x2a) & 2)) good = 0;
        }
    }
    f = fopen("CAPTURE.TXT","a");
    if (!f) return 0;
    fprintf(f,"renderer screenshot %s: %s\n",name,good ? "PASS" : "FAIL");
    if (fclose(f)) good = 0;
    return good;
}
