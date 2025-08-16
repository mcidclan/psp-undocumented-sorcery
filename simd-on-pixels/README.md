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

## More About GE as SIMD Unit

The goal of this approach is to use each bit of any 32-bit word in a data block and apply single logical operations to the full block. Logical operations are normally performed in RGB space because the 8 high-order bits are reserved for the stencil buffer. 

On the other hand, while in any pixel format mode, the GE seems to process blending on clamped RGB, which makes sense, but it makes it 'impossible' (until we find a way) to use blending as an arithmetic unit for 32 aligned bits word.

A solution would be to let a padding and process 32bits in bytes columns, which would not be bad as an alternative approach. Instead of fighting against the RGB pipeline limitations, we adapt to its natural structure.

*m-c/d*