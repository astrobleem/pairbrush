# Pairbrush

A DOS paint app for the Tandy 1000 EX. Paint with 16 native colors or browse all 256 ordered color pairs on a 320 x 200 pixel canvas.

Use a mouse or the keyboard to paint, pick colors, choose 1/2/4/8-cell brushes, pan the canvas, and undo or redo a whole stroke. Each cell contains two pixels; reversing a color pair keeps its order distinct.

![Pairbrush color-pair browser](docs/pairs.png)

## Run

Requires a Tandy 1000 EX with 640 KB RAM installed (624 KB reported by BIOS), Tandy video mode 9, and plain DOS 3 or later. Start in text mode 3 and exit Windows before running. Ordinary IBM CGA is not supported.

Run `PAINT.EXE` from a writable directory. A DOS mouse driver enables mouse painting; use `PAINT /NOMOUSE` for keyboard controls. For composite output, choose colors on the display you will use.

## Controls

- Left-drag to paint; right-click to pick a color pair.
- Arrows move the crosshair. Space stamps; D lowers or lifts the pen.
- P opens the pair browser. Arrows select, Enter accepts, and Escape cancels.
- [ and ] change brush size. F switches between the toolbar and full canvas.
- U toggles undo/redo. S saves, L loads, C clears, H opens help, and Escape quits.

Art saves as `.TDP` files in the launch directory. Choose an unused name of up to eight letters, digits, underscores or hyphens; saving never overwrites an existing file.

## Build

Build the source with Microsoft C 6.0 and DOS LINK. See [build instructions](docs/BUILD.md) and the [controls and file-format guide](docs/USER-GUIDE.md).
