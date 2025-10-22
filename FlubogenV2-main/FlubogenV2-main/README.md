# FlubogenV2

Software TODOs: https://docs.google.com/document/d/1hdh_-02SrZs_V_7GrktBjlJhYZVeI7TntKFbdAt2yxw/edit?tab=t.0

RP2040-based börd by @MyloWhylo
 * 4 WS2812 outputs
 * Serial RX/TX for remote
 * IR RX/TX
 * Boop input
 * Composite video in/out for OSD
 * uSD slot for configuration and animation files
 
Remote is Arduino Nano based, takes in finger contact inputs and outputs serial commands

Original animation tool: https://wa.funsite.cz/protogen-animator/

# Animator notes

The led_coordinates array (x,y coordinates in the order in which the LEDs are wired up) is only used during export to look up each LED color in the 2D frame array.
It is not used to draw the preview canvas, which is currently defined as two rectangles: a containing one, and a hole.

To be able to define arbitrary LED matrix shapes, use led_coordinates for both. Keep using a 2D frame array for efficiency, but have a bool attribute in each item
to know if it's part of the matrix (drawable) or not.

Load/save matrix shapes (led_coordinates array) in simple JSON files ?
Storing it in the .fur file would be simpler (unique file for animation AND matrix shape), even though the hardware doesn't need it.

Simplest way of making a matrix shape editor ? Just have a big grid with numbered coordinates shown and draw the LEDs one by one in order ?

# Files

 * .fur for animations (add LED count field, animation name field ?, and matrix shape array (for editing only))
 * .json for settings / finger mapping ? What's the easiest to parse on a MCU ?

Currently, there can only be a single .fur file with all the animations inside
How should we handle animation discovery if there are multiple .fur files each having multiple animations inside ?
Have only one animation per .fur file, and use file name ?
Have several animations per .fur file with embedded name fields, and have the controller parse everything on startup ?

# Old fur file format

 * u8: number of animations in file
 * Animation blocks
   * u8: animation speed (FPS) 0~127, MSB set means colors are indexed (less than 256)
   * Raw (24-bit colors)
   * Indexed (n-bit colors, n <= 8):
     * u8: number of colors - 1
     * List of 24-bit colors
   * u16: frame count
   * Consecutive frames of animation, with 24-bit colors (raw), or one or more color per byte (1-bit color: 8 LEDs/byte, 2-bit color: 4 LEDs/byte, 3- and 4-bit color: 2 LEDs/byte...)

# New fur file format

Improvements:
 * Header with format version number
 * Stored animation pointers, no need to compute them each time the file is read
 * Global LED matrix layout (for editing, LED count and output index for playback)
 * Per-animation name field (for editing and OSD)
 * Per-animation color format field
 * Per-frame delay like GIFs, no single FPS value, avoids having to duplicate frames to slow things down

 * Header
   * char[3]: header "FUR"
   * u8: file format version (currently 1)
   * Matrix definition (offset: 4)
     * u8: matrix output index to use (0: face, 1: cheek, ...)
     * u16: m number of LEDs in matrix
     * u8[m*2]: x and y coordinates of each LED, in the order they are wired up
 * Animation list (offset: 4 + 3 + m * 2)
   * u8: n number of animations in file
   * u32[n]: pointers to each animation block
 * Animation blocks (offset: 4 + 2 + m * 2 + n * 4)
   * char[16]: animation name
   * u16: frame count
   * u8: color format (0: raw, 1: indexed, ...)
   * Raw (24-bit colors)
   * Indexed (n-bit colors, n <= 8):
     * u8: k number of colors in palette
     * u8[k*3]: list of 24-bit colors
   * Animation frames
     * u8: post-delay in 10ms steps (0:10ms, 255:2.56s)
     * m colors: 24-bit colors (raw), or one or more colors per byte (1-bit color: 8/byte, 2-bit color: 4 /byte, 3- and 4-bit color: 2/byte...)
