# xflipbook
Bloat-free animation program using SDL2, GL, Xi, and ImGui

## Building and running
Dependencies:
 - GLEW
 - SDL2
 - libX11, libXi

To run: `make; ./main`

## Shortcuts
 - q – quit
 - , – previous frame
 - . – next frame
 - space – play/stop
 - [ – toggle onion skin (backward)
 - ] – toggle onion skin (forward)
 
## Troubleshooting
If your pen is drawing at the wrong position or isn't drawing anything at all, try the following.

In `main.cpp` change constant 16777216 according to your tablet area settings.

For some configurations, it should be set at around 32000. To get more precise info you can use `xsetwacom`.
