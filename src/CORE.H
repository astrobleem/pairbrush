/* Pairbrush document core. C89; shared verbatim by DOS and host QA. */
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <io.h>
#ifndef far
#ifdef HOST_QA
#define far
#endif
#endif
#define DOC_W 160
#define DOC_H 200
#define DOC_N 32000U
typedef unsigned char BYTE;
static BYTE far *document, far *undo_image, far *load_image;
static int undo_valid, modified, undo_modified;
static unsigned color_pair = 0xff;
static int brush_size = 2;
static int dirty_left, dirty_top, dirty_right, dirty_bottom;
static unsigned video_offset(int x, int y)
{
    return (unsigned)((y & 3) * 8192U + (y >> 2) * 160U + (x >> 1));
}
static void dirty_reset(void)
{ dirty_left = DOC_W; dirty_top = DOC_H; dirty_right = dirty_bottom = -1; }
static void dirty_add(int x, int y)
{
    if (x < dirty_left) dirty_left = x;
    if (x > dirty_right) dirty_right = x;
    if (y < dirty_top) dirty_top = y;
    if (y > dirty_bottom) dirty_bottom = y;
}
static void image_copy(BYTE far *dst, const BYTE far *src)
{
    unsigned i;
    for (i = 0; i < DOC_N; i++) dst[i] = src[i];
}
static void snapshot(void)
{
    image_copy(undo_image, document);
    undo_modified = modified; undo_valid = 1;
}
static void undo_swap(void)
{
    unsigned i; BYTE b; int m;
    if (!undo_valid) return;
    for (i = 0; i < DOC_N; i++) {
        b = document[i]; document[i] = undo_image[i]; undo_image[i] = b;
    }
    m = modified; modified = undo_modified; undo_modified = m;
    dirty_left = dirty_top = 0;
    dirty_right = DOC_W-1; dirty_bottom = DOC_H-1;
}
static void stamp(int x, int y)
{
    int xx, yy, left, top; unsigned pos;
    left = x - (brush_size-1)/2; top = y - (brush_size-1)/2;
    for (yy = top; yy < top+brush_size; yy++) {
        if (yy < 0 || yy >= DOC_H) continue;
        for (xx = left; xx < left+brush_size; xx++) {
            if (xx < 0 || xx >= DOC_W) continue;
            pos = (unsigned)(yy * DOC_W + xx);
            if (document[pos] != (BYTE)color_pair) {
                document[pos] = (BYTE)color_pair;
                dirty_add(xx, yy); modified = 1;
            }
        }
    }
}
static void stroke(int x0, int y0, int x1, int y1)
{
    int dx, dy, sx, sy, err, e2;
    dx = x1-x0; if (dx < 0) dx = -dx;
    dy = y1-y0; if (dy < 0) dy = -dy;
    sx = x0 < x1 ? 1 : -1; sy = y0 < y1 ? 1 : -1;
    err = dx-dy;
    for (;;) {
        stamp(x0,y0); if (x0 == x1 && y0 == y1) break;
        e2 = 2*err;
        if (e2 > -dy) { err -= dy; x0 += sx; }
        if (e2 < dx) { err += dx; y0 += sy; }
    }
}
static void clear_document(void)
{
    unsigned i;
    snapshot();
    for (i = 0; i < DOC_N; i++) document[i] = 0;
    modified = 1; dirty_left = dirty_top = 0;
    dirty_right = DOC_W-1; dirty_bottom = DOC_H-1;
}
static unsigned image_crc(const BYTE far *src)
{
    static const unsigned table[16] = {
        0x0000,0x1021,0x2042,0x3063,0x4084,0x50a5,0x60c6,0x70e7,
        0x8108,0x9129,0xa14a,0xb16b,0xc18c,0xd1ad,0xe1ce,0xf1ef
    };
    unsigned crc = 0xffff, i, c;
    for (i = 0; i < DOC_N; i++) {
        c = src[i];
        crc = ((crc << 4) ^ table[((crc >> 12) ^ (c >> 4)) & 15])
              & 65535U;
        crc = ((crc << 4) ^ table[((crc >> 12) ^ c) & 15]) & 65535U;
    }
    return crc;
}
static void put_word(BYTE *p, unsigned n)
{ p[0] = (BYTE)n; p[1] = (BYTE)(n >> 8); }
static unsigned get_word(const BYTE *p)
{ return (unsigned)(p[0] | ((unsigned)p[1] << 8)); }
/* Never opens an existing file for writing. A short failed new file is removed.
 * Return 1 success, 2 already exists, 0 I/O failure. */
static int save_document(const char *name)
{
    FILE *f; int fd, bad = 0; unsigned i; BYTE header[16];
    f = fopen(name,"rb"); if (f) { fclose(f); return 2; }
    fd = open(name,O_WRONLY|O_CREAT|O_EXCL|O_BINARY,S_IREAD|S_IWRITE);
    if (fd < 0) return 0;
    f = fdopen(fd,"wb");
    if (!f) { close(fd); remove(name); return 0; }
    memcpy(header,"TPB1",4);
    put_word(header+4,DOC_W); put_word(header+6,DOC_H);
    put_word(header+8,DOC_N); put_word(header+10,image_crc(document));
    put_word(header+12,0); put_word(header+14,0);
    if (fwrite(header,1,16,f) != 16) bad = 1;
    for (i = 0; !bad && i < DOC_N; i++)
        if (fputc(document[i],f) == EOF) bad = 1;
    if (fclose(f)) bad = 1;
    if (bad) { remove(name); return 0; }
    modified = 0;
    /* Undo remains useful, but its other state differs from the saved image. */
    if (undo_valid) undo_modified = 1;
    return 1;
}
static int load_document(const char *name)
{
    FILE *f; BYTE header[16]; unsigned i; int c, bad = 0;
    f = fopen(name,"rb"); if (!f) return 0;
    if (fread(header,1,16,f) != 16 || memcmp(header,"TPB1",4) ||
        get_word(header+4) != DOC_W || get_word(header+6) != DOC_H ||
        get_word(header+8) != DOC_N || get_word(header+12) ||
        get_word(header+14)) { fclose(f); return 0; }
    for (i = 0; i < DOC_N; i++) {
        c = fgetc(f); if (c == EOF) { bad = 1; break; }
        load_image[i] = (BYTE)c;
    }
    if (fgetc(f) != EOF || ferror(f)) bad = 1;
    if (fclose(f)) bad = 1;
    if (bad || image_crc(load_image) != get_word(header+10)) return 0;
    snapshot(); image_copy(document,load_image); modified = 0;
    dirty_left = dirty_top = 0;
    dirty_right = DOC_W-1; dirty_bottom = DOC_H-1;
    return 1;
}
