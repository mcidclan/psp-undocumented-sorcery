## VFPU Simple Rendering Code Sample

This sample shows how to use the PSP's VFPU to render pixels directly to VRAM, using vector operations and format conversion.

An array of 8×4 RGBA colors is loaded into VFPU registers using general-purpose Allegrex loads.
Each column (4 pixels high) is swizzled upward (`Y→X, Z→Y, W→Z, X→W`).
After swizzling, the bottom values of each column are shifted into the next one, creating a circular movement across all 8 columns.

The line is **32 pixels wide and 4 pixels high**, and each 4×4 block moves horizontally from right to left in a cycle.

Once transformed, colors are converted from **RGBA8888 to RGB5650** using VFPU SIMD instructions (`vt5650.q`).
Finally, the pixels are written to VRAM in groups of 8 using `sv.q` instructions within a branch-controlled iteration.

## Disclaimer

This project and code are provided as-is without warranty. Users assume full responsibility for any implementation or consequences. Use at your own discretion and risk.

*m-c/d*
