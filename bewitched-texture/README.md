# PSP Bewitched Texture Trick
This repo demonstrates **two different approaches** to achieve the same result: bypassing PSPGU's alpha channel write limitations. Both methods force alpha channel values to be set, however with limitations. Indeed, the alpha output will be that of the controlling texture, which we refer to here as the "bewitched texture".

**main_565.cpp**: This sample draws a `bewitched` texture using 5650 pixel format, forcing the alpha channel values to be set. This texture is then blended with an 8888 texture and output to the frame buffer. The resulting output texture will have its alpha values matching those of the `bewitched` texture, while the RGB values are blended between both textures.

**main_gu_points.cpp**: This sample draws a `bewitched` texture using GU_POINTS through `Clear` mode, forcing the alpha channel values to be set. This texture is then blended with an 8888 standard texture and output to the frame buffer. The resulting output texture will have its alpha values matching those of the `bewitched` texture, while the RGB values are blended between both textures.

**Note**: The code provides example tools for selecting which texture will control the alpha channel.

## Important Notice
These demonstrations are **not presented as universal solutions**, but rather as a foundation for improvement or reflection for other possible 'hacks'. These are **experimental techniques** that fall within the scope of the **PSP Undocumented Sorcery** project.

## Compilation
```bash
make MODE=565 clean; make MODE=565;                 # Compile with RGB565 method (default)
make MODE=GU_POINTS clean; make MODE=GU_POINTS ;    # Compile with GU_POINTS method
```

## Disclamer
This project and code are provided as-is without warranty. Users assume full responsibility for any implementation or consequences. Use at your own discretion and risk

*m-c/d*
