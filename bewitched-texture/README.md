# PSP Bewitched Texture Blending

This repo demonstrates **three different approaches** to bypassing PSPGU's alpha channel write limitations. All methods force alpha channel values to be set, however with limitations. Indeed, the alpha output for 565 and GU_POINTS methods will be that of the controlling texture, which we refer to here as the "bewitched texture". And the Isolated alpha method will require several texture processing passes and tricks to achieve true alpha blending.

**main.cpp**: This sample draws a `bewitched` texture by isolating the alpha components, blending them and then forcing the resulting value to be set in the final blended texture, while the RGB values are blended between both textures.

**main_565.cpp**: This sample draws a `bewitched` texture using 5650 pixel format, forcing the alpha channel values to be set. This texture is then blended with an 8888 texture and output to the frame buffer. The resulting output texture will have its alpha values matching those of the `bewitched` texture, while the RGB values are blended between both textures.

**main_gu_points.cpp**: This sample draws a `bewitched` texture using GU_POINTS through `Clear` mode, forcing the alpha channel values to be set. This texture is then blended with an 8888 standard texture and output to the frame buffer. The resulting output texture will have its alpha values matching those of the `bewitched` texture, while the RGB values are blended between both textures.

**Note**: The code provides example tools for selecting which texture will control the alpha channel.

## Important Notice

These demonstrations are **not presented as universal solutions**, but rather as a foundation for improvement or reflection on other possible 'hacks'. These are **experimental techniques** that fall within the scope of the **PSP Undocumented Sorcery** project.

## Compilation

```bash
make clean; make;                                   # Compile with Isolated Alpha method (default)
make MODE=565 clean; make MODE=565;                 # Compile with RGB565 method
make MODE=GU_POINTS clean; make MODE=GU_POINTS;     # Compile with GU_POINTS method
```

## Running The Project

All samples have been tested on Slim, and the main one (Isolated Alpha) has been tested on Fat and 3000 in addition to Slim.  
  
***Do not test this on an emulator as the results may not be as expected.***  

## Disclaimer

This project and code are provided as-is without warranty. Users assume full responsibility for any implementation or consequences. Use at your own discretion and risk.

*m-c/d*
