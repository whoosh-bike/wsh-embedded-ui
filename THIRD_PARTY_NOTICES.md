# Third-party notices

The MIT license in [LICENSE](LICENSE) covers the weui sources. The components below keep
their own licenses.

## u8g2

`external/fonts/u8g2.c`, `external/fonts/u8g2.h` and `external/fonts/u8g2_fonts.c` are a
trimmed copy of the [u8g2](https://github.com/olikraus/u8g2) font renderer.

Copyright (c) 2016, olikraus@gmail.com
All rights reserved.

Redistribution and use in source and binary forms, with or without modification, are
permitted provided that the following conditions are met:

- Redistributions of source code must retain the above copyright notice, this list of
  conditions and the following disclaimer.
- Redistributions in binary form must reproduce the above copyright notice, this list of
  conditions and the following disclaimer in the documentation and/or other materials
  provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY
EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL
THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR
TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

## Fonts in u8g2_fonts.c

`external/fonts/u8g2_fonts.c` is generated font data. Every font block in it carries its
own `Fontname`, `Copyright` and (where applicable) `License` fields. Those notices are the
authoritative ones for that font; the MIT license of this repository does not replace them.

The set includes public-domain fonts, SIL Open Font License 1.1 fonts, Apache-2.0 fonts and
fonts offered under GPLv2+ with the GNU Font Embedding Exception. Where a font is dual
licensed, use it under the license that fits your product. The OFL text is in
[LICENSES/OFL-1.1.txt](LICENSES/OFL-1.1.txt).

The full font set is kept so applications can pick a font without rebuilding the library.
If you need a smaller, single-license distribution, trim `u8g2_fonts.c` to the fonts you
actually ship and keep the matching notices.

## JetBrains Mono (lab only)

`tools/lab/public/fonts/*.woff2` are JetBrains Mono, Copyright 2020 The JetBrains Mono
Project Authors, licensed under the SIL Open Font License 1.1. The license text is in
[LICENSES/OFL-1.1.txt](LICENSES/OFL-1.1.txt).

## Optional dependencies

These are not part of this repository; add them to the host build if you use the feature.

| Component | Used for | License |
|---|---|---|
| [qrcodegen](https://github.com/nayuki/QR-Code-generator) | QR widget | MIT |
| [jsmn](https://github.com/zserge/jsmn) | asset metadata | MIT |
| [LZ4](https://github.com/lz4/lz4) | compressed images | BSD-2-Clause |
| wsh-embedded-libs | base types, math, containers | Whoosh internal, provided by the host |

## npm dependencies (lab only)

`tools/lab` uses Vite and TypeScript. Their licenses are listed in the packages installed
from `tools/lab/package-lock.json`; they are development tools and are not shipped in
firmware.
