/* Build-only fixture includes the exact production UI with a screenshot hook.
 * PAINT.EXE is compiled separately without this adapter. */
#define CAPTURE_QA 1
#include "CAPTURE.H"
#define main paint_main
#include "PAINT.C"
#undef main
int main(int argc, char **argv) { return paint_main(argc,argv); }
