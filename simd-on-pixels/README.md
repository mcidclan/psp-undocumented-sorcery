# PSP GE SIMD Pixel Logical Operations Sample
This sample demonstrates how to exploit the PlayStation Portable's Graphics Engine (GE) to perform SIMD logical operations on pixel data.  
## How It Works
This technique exploits the GE's pixel processing pipeline to achieve SIMD-style computations. It implements a two-layer processing approach:
### Layer 0: SIMD Masking
- Uses `8888` color format for initial data setup
- Applies a pixel mask (`0x000000FF`) that masks the lower 8 bits of each 32-bit word
- Processes 4 data points with specific bit patterns
- Processes through clearing mode to ensure all bits can be affected
### Layer 1: SIMD Logical Operations  
- Switches to `565` color format to preserve all bits during operations
- Applies logical OR operation as a proof of concept
- Processes the logical operation between 8 `565` data points against the previous data

Note: This operates on each bit including the 8 high-order bits
## Output
The program displays the result for each word via pspDebugScreenPrint after SIMD processing.
It needs to be executed on physical hardware to make sure to get the print output.  

*m-c/d*
