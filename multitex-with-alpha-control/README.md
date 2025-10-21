# PSPGU Multi Texturing With Alpha Variation

This demo sample shows how a mask can be applied to the alpha channel, and demonstrates varying the alpha value through that mask. The texture is rendered in 2 passes and combined with a direct multiplicative blend as follows:  

```c
sceGuBlendFunc(GU_ADD, GU_DST_COLOR, GU_FIX, 0, 0);
sceGuTexFunc(GU_TFX_MODULATE, GU_TCC_RGBA);
````

The trick lies in the fact that, using the gu, the **alpha component** of 32-bit or 16-bit colors is used as storage for **stencil bits**. By taking this into account, we can manipulate these bits (through `sceGuPixelMask` and stencil clears) to affect the alpha component.

Note: This has been tested on real hardware (PSP Slim) and debug information may not display correctly in emulators.

## Disclamer
This project and code are provided as-is without warranty. Users assume full responsibility for any implementation or consequences. Use at your own discretion and risk

*m-c/d*
