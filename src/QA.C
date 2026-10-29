/* Same document engine tests on modern host and actual MSC6 DOS target. */
#include <stdlib.h>
#ifdef HOST_QA
#define far
#else
#include <dos.h>
#define MK_FP(s,o) ((void far *)(((unsigned long)(s) << 16) | (unsigned)(o)))
#endif
#include "CORE.H"
static BYTE far *buffers[3];
static unsigned long checks;
static void check(int ok, const char *name)
{
    checks++;
    if (!ok) { printf("FAIL %s at check %lu\n",name,checks); exit(1); }
}
static void zero(void)
{
    unsigned i;
    for (i = 0; i < DOC_N; i++) document[i] = 0;
    modified = 0; undo_valid = 0; dirty_reset();
}
static void guards(void)
{
    unsigned i, j;
    for (j = 0; j < 3; j++) for (i = 0; i < 8; i++) {
        check(buffers[j][i] == 0xa5,"leading canary");
        check(buffers[j][DOC_N+8+i] == 0xa5,"trailing canary");
    }
}
static void fixture(const char *name, unsigned length, int mutate, int extra)
{
    FILE *in, *out; unsigned i; int c;
    in = fopen("QAONE.TDP","rb"); out = fopen(name,"wb");
    check(in != NULL && out != NULL,"fixture open");
    for (i = 0; i < length; i++) {
        c = fgetc(in); check(c != EOF,"fixture source length");
        if ((int)i == mutate) c ^= 1;
        check(fputc(c,out) != EOF,"fixture write");
    }
    if (extra) fputc(0,out);
    check(!fclose(in) && !fclose(out),"fixture close");
}
int main(void)
{
    unsigned i, j, pair, crc, before, pos, undo_before;
    int x, y, sz, xx, yy, expected, old_modified;
    BYTE header[16]; FILE *f;
    static unsigned lengths[] = {0,1,3,4,8,15,16,17,100,16016,32015};
    static int mutations[] = {0,4,6,8,10,12,14,16,16016,32015};
    int corners[4][2];
#ifndef HOST_QA
    unsigned segments[3];
#endif
    for (j = 0; j < 3; j++) {
#ifdef HOST_QA
        buffers[j] = (BYTE *)malloc(DOC_N+16);
        check(buffers[j] != NULL,"host allocation");
#else
        check(!_dos_allocmem(2001,&segments[j]),"DOS allocation");
        buffers[j] = (BYTE far *)MK_FP(segments[j],0);
#endif
        for (i = 0; i < DOC_N+16; i++) buffers[j][i] = 0xa5;
    }
    document = buffers[0]+8; undo_image = buffers[1]+8;
    load_image = buffers[2]+8;
    zero(); brush_size = 1;
    /* Each cell byte is A in the high/even nibble, B in the low/odd nibble.
     * Every ordered pair is stamped at multiple absolute x/y positions. */
    for (pair = 0; pair < 256; pair++) {
        color_pair = pair;
        for (i = 0; i < 160; i++) {
            y = (int)(i+pair)%200; stamp((int)i,y);
            pos = (unsigned)(y*160+i);
            check((unsigned)document[pos] == pair,"all 256 pairs at all x");
            check((unsigned)(document[pos] >> 4) == pair/16,"absolute even A");
            check((unsigned)(document[pos] & 15) == pair%16,"absolute odd B");
        }
    }
    for (y = 0; y < 200; y++) for (x = 0; x < 320; x += 2) {
        check(video_offset(x,y) == (unsigned)((y%4)*8192+(y/4)*160+x/2),
              "four-bank address");
        check(video_offset(x,y) == video_offset(x+1,y),"pair single byte");
    }
    corners[0][0] = 0; corners[0][1] = 0;
    corners[1][0] = 159; corners[1][1] = 0;
    corners[2][0] = 0; corners[2][1] = 199;
    corners[3][0] = 159; corners[3][1] = 199;
    for (sz = 1; sz <= 8; sz *= 2) for (j = 0; j < 4; j++) {
        zero(); brush_size = sz; color_pair = 0x42;
        x = corners[j][0]; y = corners[j][1]; stamp(x,y);
        for (yy = 0; yy < 200; yy++) for (xx = 0; xx < 160; xx++) {
            expected = xx >= x-(sz-1)/2 && xx < x-(sz-1)/2+sz &&
                       yy >= y-(sz-1)/2 && yy < y-(sz-1)/2+sz;
            check((int)document[yy*160+xx] == (expected ? 0x42 : 0),
                  "brush edge clipped rectangle");
        }
        guards();
    }
    zero(); brush_size = 1; color_pair = 0xf0;
    snapshot(); stroke(0,0,159,199);
    check(document[0] == 0xf0 && document[31999] == 0xf0,"stroke endpoints");
    for (y = 0; y < 200; y++) {
        expected = 0;
        for (x = 0; x < 160; x++) if (document[y*160+x]) expected++;
        check(expected >= 1 && expected <= 2,"continuous stroke every row");
    }
    crc = image_crc(document); undo_swap();
    for (i = 0; i < DOC_N; i++) check(!document[i],"undo whole stroke");
    check(!modified,"undo returns clean state");
    undo_swap(); check(image_crc(document) == crc,"redo exact bytes");
    clear_document(); undo_swap();
    check(image_crc(document) == crc,"undo clear");
    /* A deterministic fixture containing every byte value. */
    for (i = 0; i < DOC_N; i++) document[i] = (BYTE)(i & 255);
    modified = 1; crc = image_crc(document);
    check(save_document("QAONE.TDP") == 1,"first save");
    check(!modified,"saved state");
    document[0] ^= 255; modified = 1;
    check(save_document("QAONE.TDP") == 2,"existing file kept");
    check(modified,"refused save retains edited flag");
    check(load_document("QAONE.TDP"),"load valid");
    check(image_crc(document) == crc && !modified,"exact load");
    undo_swap(); check(document[0] == 255,"load undo restores previous art");
    undo_swap(); check(image_crc(document) == crc,"load redo");
    check(save_document("QATWO.TDP") == 1,"second save new name");
    for (j = 0; j < 3; j++) {
        check(load_document("QAONE.TDP"),"repeat load one");
        check(load_document("QATWO.TDP"),"repeat load two");
        check(image_crc(document) == crc,"repeat identical");
    }
    before = image_crc(document);
    undo_before = image_crc(undo_image); old_modified = modified;
    for (j = 0; j < sizeof(lengths)/sizeof(lengths[0]); j++) {
        fixture("QABAD.TDP",lengths[j],-1,0);
        check(!load_document("QABAD.TDP"),"truncated rejected");
        check(image_crc(document) == before,"truncated art unchanged");
        check(image_crc(undo_image) == undo_before && modified == old_modified,
              "truncated undo/state unchanged");
    }
    for (j = 0; j < sizeof(mutations)/sizeof(mutations[0]); j++) {
        fixture("QABAD.TDP",32016,mutations[j],0);
        check(!load_document("QABAD.TDP"),"header/CRC/body damage rejected");
        check(image_crc(document) == before,"damaged art unchanged");
        check(image_crc(undo_image) == undo_before && modified == old_modified,
              "damaged undo/state unchanged");
    }
    fixture("QABAD.TDP",32016,-1,1);
    check(!load_document("QABAD.TDP"),"trailing byte rejected");
    check(image_crc(document) == before,"oversized art unchanged");
    check(image_crc(undo_image) == undo_before && modified == old_modified,
          "oversized undo/state unchanged");
    check(!load_document("MISSING.TDP"),"missing file rejected");
    f = fopen("QAONE.TDP","rb"); check(f != NULL,"header read open");
    check(fread(header,1,16,f) == 16,"header read"); fclose(f);
    check(get_word(header+10) == crc,"CRC header exact");
    guards();
    remove("QAONE.TDP"); remove("QATWO.TDP"); remove("QABAD.TDP");
    for (j = 0; j < 3; j++) {
#ifdef HOST_QA
        free(buffers[j]);
#else
        check(!_dos_freemem(segments[j]),"DOS free");
#endif
    }
    printf("PASS %lu document/pair/phase/edge/undo/file assertions\n",checks);
    return 0;
}
