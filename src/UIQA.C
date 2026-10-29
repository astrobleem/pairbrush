#define HOST_QA 1
#define HOST_UI
#define main paint_main
#include "PAINT.C"
#undef main
static unsigned ui_checks;
static void check_ui(int yes, const char *s)
{
    ui_checks++;
    if (!yes) { printf("FAIL UI %s\n",s); exit(1); }
}
int main(void)
{
    char *args[3]; int x, y, pair, i; unsigned before;
    BYTE b;
    host_init(); video = (BYTE *)MK_FP(0xb800,0);
    args[0] = "PAINT"; args[1] = "/QA"; args[2] = "/NOMOUSE";
    check_ui(paint_main(3,args) == 0,"scripted app path");
    check_ui(!host_errors && !host_blocks[0] && !host_blocks[1] &&
             !host_blocks[2],"no unexpected host IO or leaked blocks");
    /* The actual production rendering code runs over a host-backed VRAM.
     * Swatch centers must contain all 256 exact pair bytes. */
    browser = 1; color_pair = 0x0f; cursor_on = 0; browser_draw();
    for (pair = 0; pair < 256; pair++) {
        x = 20+(pair & 15)*18; y = 34+(pair >> 4)*9;
        for (i = 0; i < 7; i++) {
            b = video[video_offset(x+6,y+i)];
            check_ui(b == pair,"256 swatch byte identity");
        }
        check_ui(!(x & 1),"swatch absolute phase");
    }
    browser = 0; full_view = 0; view_x = 8; view_y = 64;
    for (i = 0; i < 3; i++) {
        check_ui(!_dos_allocmem(2000,&image_segment[i]),"reallocate UI docs");
    }
    document = MK_FP(image_segment[0],0);
    undo_image = MK_FP(image_segment[1],0);
    load_image = MK_FP(image_segment[2],0);
    for (i = 0; i < 32000; i++) document[i] = (BYTE)i;
    screen_draw();
    for (y = 0; y < 136; y++) for (x = 0; x < 152; x++) {
        b = video[video_offset(8+x*2,24+y)];
        check_ui(b == document[(y+64)*160+x+8],"panned canvas phase");
    }
    before = image_crc(document); pointer_x = 319; pointer_y = 199;
    cursor_show(); cursor_hide();
    check_ui(image_crc(document) == before,"pointer never enters document");
    command('P'); command(333); command(336); command(27);
    check_ui(color_pair == browser_original,"Escape cancels pair selection");
    color_pair = 0x6e; brush_size = 1; undo_valid = modified = 0;
    mouse_input(40,44,1); mouse_input(0,0,1); mouse_input(70,70,1);
    mouse_input(70,70,0); undo_swap();
    check_ui(image_crc(document) == before,"leaving canvas retains whole stroke undo");
    cursor_x = 159; cursor_y = 199; keyboard_pointer();
    check_ui(view_x == 8 && view_y == 64,"automatic lower-right pan");
    cursor_x = cursor_y = 0; keyboard_pointer();
    check_ui(view_x == 0 && view_y == 0,"automatic upper-left pan");
    mouse_report(65535U,65535U,0);
    check_ui(last_mx == 319 && last_my == 199,"mouse words clipped before cast");
    mouse_report(0,0,4);
    check_ui(last_mx == 0 && last_my == 0 && !last_buttons,
             "mouse origin and unused buttons bounded");
    full_view = 1; screen_draw();
    for (y = 0; y < 200; y++) for (x = 0; x < 160; x++)
        check_ui(video[video_offset(x*2,y)] == document[y*160+x],
                 "full view exact bytes");
    restore(); check_ui(!host_errors,"cleanup host errors");
    printf("PASS %u production UI rendering/input assertions (host adapter)\n",ui_checks);
    return 0;
}
