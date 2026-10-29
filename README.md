# Pairbrush

A small DOS paint app for the Tandy 1000 EX. Paint with all 256 ordered pairs of the 16 native colors on a 160 x 200 two-pixel-cell canvas (320 x 200 screen pixels).

Pairbrush supports mouse and keyboard painting, color picking, 1/2/4/8-cell brushes, a panning toolbar view, full canvas view, whole-stroke undo/redo, and checked `.TDP` save/load files. Choose colors on the actual display; no composite-color calibration is assumed.

![Pair browser](docs/pairs.png)

The ordered-pair browser in DOSBox-X. Actual composite colors depend on the display.

## Build and run

Build using Microsoft C 6.0 and DOS LINK; see [build instructions](docs/BUILD.md).

Run `PAINT.EXE` in plain DOS 3 or later on a Tandy 1000 EX with 640 KB installed (624 KB reported by BIOS), starting in text mode 3. Exit Windows first. A DOS mouse driver is optional; `PAINT /NOMOUSE` uses keyboard controls. Requires Tandy video mode 9; ordinary IBM CGA is not supported.

See [controls and file format](docs/USER-GUIDE.md).
