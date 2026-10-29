# Controls and files

- Left drag paints; right click picks the exact ordered pair.
- Arrows move the crosshair. Space stamps; D lowers/lifts the pen for continuous strokes.
- P opens the 256-pair browser. Arrows select; Enter/P accepts; Escape cancels.
- Click the native swatch strip for solid colors. Click SIZE or use [ and ] for 1/2/4/8-cell brushes.
- F switches full canvas/toolbar view; U toggles undo/redo; S saves; L loads; C clears with confirmation; H shows help; Escape quits with confirmation when edited.

Each document byte is an ordered AB color pair: A occupies an absolute even pixel, B the next odd pixel. AB and BA remain distinct. AA pairs are native solids. The document is 160 x 200 cells, or 320 x 200 physical pixels. The toolbar viewport is 152 x 136 cells and pans with the keyboard crosshair.

Save/load names contain 1–8 letters, digits, underscores or hyphens. Files stay in the launch directory as NAME.TDP. Saving refuses existing files. Invalid loads preserve the current art; a successful load can be undone.

TDP files contain exactly 32,016 bytes: `TPB1`, then six little-endian 16-bit words (width 160, height 200, body length 32,000, body CRC16-CCITT, flags 0, reserved 0), then 32,000 row-major AB bytes. CRC starts at FFFF, uses polynomial 1021 and no final XOR. A disk or power failure may leave a partial new save if DOS cannot remove it.
