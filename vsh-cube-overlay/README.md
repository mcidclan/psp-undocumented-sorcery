# VSH Cube Overlay - Plugin

A VSH plugin that displays a rotating 3D cube overlay on top of the XMB using PSPGU.

## Technical Details

### VRAM Management by Model

The VSH actively uses VRAM through parallel threads, which creates specific constraints for depth buffer management in overlayed 3D graphics. Since we hook `setDisplayBuffer` which executes in one thread, while VRAM is potentially accessed by other threads, careful placement is crucial to avoid concurrency issues and/or data corruption.

#### PSP Slim/3000+ (Extended VRAM ≥ 0x200000)

These models have additional VRAM, allowing the depth buffer to be placed at `0x200000` without apparent overlap with other VRAM data buffers. However, testing has not been performed on PSP Go, and an analysis of `paf.prx` would ensure complete safety across all models.

#### PSP Fat (Standard VRAM < 0x200000)
On these models, VRAM space is limited which imposes a sorcery:

1. The second half of the framebuffer (starting at `0x44000`) is copied to a temporary buffer
2. This area temporarily becomes our depth buffer for 3D rendering
3. After rendering, the area is restored with the saved data

**Consequences:**
- Constant CPU boost in VSH to compensate for performance loss due to copy operations
- Limited rendering area: The cube/3D model must stay within the first vertical half of the framebuffer
- More verbose but safe

### Special Thanks
Thanks to Acid_Snake and Isage for their time and dedication.

## Disclaimer
This project and code are provided as-is without warranty. Users assume full responsibility for any implementation or consequences. Use at your own discretion and risk.

*m-c/d*


