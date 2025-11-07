# VSH Cube Overlay - Plugin
A VSH plugin that displays a rotating 3D cube overlay on top of the XMB using PSPGU.

## Technical Details

### VRAM Management and Depth Buffer Strategy

The VSH actively uses VRAM through parallel threads, which creates specific constraints for depth buffer management in overlayed 3D graphics. Since we hook `setDisplayBuffer` which executes in one thread, while VRAM is potentially accessed by other threads, careful placement is crucial to avoid concurrency issues and/or data corruption.

#### Generic Approach

We use a unified strategy across all PSP models for two reasons:
- PSP Fat models have limited VRAM (< 0x200000), making extended VRAM placement impossible
- Even on models with extended VRAM, we cannot guarantee that the VSH won't access memory beyond the standard VRAM range, making this approach cleaner and safer

**Implementation:**

The depth buffer is placed at the beginning of the current framebuffer (offset 0x0 relative to the framebuffer address). Since the framebuffer address is dynamic and provided by the VSH's `setDisplayBuffer` call, our depth buffer location follows it. A region of 512×32 pixels is saved to a temporary backup buffer before rendering, then restored afterward.

**Rendering Optimizations:**

- **Optimized copy operations:** Only the depth buffer region (512×32 pixels) is saved and restored, matching the 64-pixel width of the depth buffer
- **Frame-split processing:** Depth buffer operations (clear) are processed in the first half of the frame, while drawing occurs in the second half
- **Depth buffer dimensions:** The depth buffer width (64 pixels) matches exactly the rendering area width to ensure proper depth testing
- **Scissor test:** A scissor region is applied to constrain rendering to a specific 64×64 area on screen
- **Threads Scheduler:** We ensure the thread scheduler is ready before processing any GU commands in the hook

**Consequences:**

- Limited rendering area: The cube is rendered in a constrained region of the screen

### Special Thanks
Thanks to Acid_Snake and Isage for their time and dedication.

## Disclaimer
This project and code are provided as-is without warranty. Users assume full responsibility for any implementation or consequences. Use at your own discretion and risk.

*m-c/d*


