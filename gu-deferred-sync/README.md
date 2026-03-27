## PSPGU Deffered Sync
This project demonstrates how to implement deferred synchronization using the pspgu finish interrupt. It uses four 565 buffers, over which the cursor is moved for each new frame, avoiding blocking the main CPU and allowing the GE to draw uninterrupted, running in parallel as much as possible.  
In this case, the GE has enough time to fill all four buffers because each frame takes very little time. Therefore, the frame time is limited to a minimum of 1000 microseconds. If the frame time goes too low, it would be necessary to increase the number of back buffers, which could lead to VRAM limitations.

## Disclamer
This project and code are provided as-is without warranty. Users assume full responsibility for any implementation or consequences. Use at your own discretion and risk

## Special thanks
To IridescentRose for highlighting the core idea.

`m-c/d`

