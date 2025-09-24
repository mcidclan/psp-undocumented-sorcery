# Scrolling a 512x512 viewport over a 1024x1024 image

This sample demonstrates how to implement a scrollable viewport of a 512x512 size on top of a 1024x1024 image using pspgu. The full image is loaded as a single compact buffer in memory. Then we perform movement by varying the offset of the texture source address with the stride parameter set to 1024.

## Build

```bash
make clean; make;
```

## Usage
Put the EBOOT in the same folder as the 1024.png image.  

Use the directional pad to move over the image.  
Use triangle and cross to zoom in or out.  
Use Home to exit.  

## Disclamer
This project and code are provided as-is without warranty. Users assume full responsibility for any implementation or consequences. Use at your own discretion and risk

*m-c/d*
