# Credits and source record

Pairbrush was made for Chad's Tandy 1000 EX / Sony WEGA setup. Chad selected MIT for its original code on 2026-10-29.

This repository copies the final 2026-10-13 R2 C sources, headers and BUILD.BAT unchanged from the existing private R2 package. `validation/FINALPIN.JSON` retains the historical source and executable identities; `source-sha256.json` records all ten included source/build files. The full historical archive and working card copy remain separate and unchanged.

Microsoft C 6.0 / LINK supplied the original DOS build; GCC supplies host checks. DOSBox-X supplied historical emulation and renderer captures. The test-only capture adapter references the [DOSBox-X integration interface](https://github.com/joncampbell123/dosbox-x/blob/dosbox-x-v2025.12.01/include/iglib.h).

The embedded 5x7 table has no recorded upstream attribution. Similar glyphs occur in [luma.core's LCD font](https://github.com/rm-hull/luma.core/blob/master/luma/core/legacy/font.py), but actual derivation was not established; no upstream authorship claim is made. The table is unchanged. MIT for the original program does not relicense third-party rights.

Compiler tools, vendor headers/libraries and compiled runtime code are not included. The historical DOS executable's SHA-256 is `7E7D04F987ADC67B3B923BA1D70956E82F218ACCD896802B475FB4C3797A6C2C` (20,093 bytes). Obtain a toolchain separately to build the included source.
