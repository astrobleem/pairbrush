# Pairbrush

A small DOS paint app for Chad's Tandy 1000 EX and Sony WEGA setup. Paint with all 256 ordered pairs of the 16 native colors on a 160 x 200 two-pixel-cell canvas.

Pairbrush supports mouse and keyboard painting, color picking, 1/2/4/8-cell brushes, a panning toolbar view, full canvas view, whole-stroke undo/redo, and checked `.TDP` save/load files. Choose colors on the actual display; no composite-color calibration is assumed.

![Pair browser](docs/pairs.png)

Historical DOSBox-X renderer capture from the production-source test fixture with synthetic input. This is an emulator image; actual composite colors depend on the display.

## Build and run

Build using Microsoft C 6.0 and DOS LINK; see [build instructions](docs/BUILD.md).

Run the built `PAINT.EXE` in plain Tandy DOS 3+, starting in mode 3, with 640 KB installed (624 KB reported by BIOS). Exit Windows first. A DOS mouse driver is optional; `PAINT /NOMOUSE` uses keyboard controls. This revision uses Tandy mode 9 and has no ordinary IBM CGA backend.

See [controls and file format](docs/USER-GUIDE.md).
