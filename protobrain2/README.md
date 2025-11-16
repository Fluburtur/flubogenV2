This is intended to be opened in Visual Studio Code, using the Raspberry Pi Pico extension and the
Raspberry Pi debug probe.

I did this on Linux. If the project settings are bad for Windows, I'm sorry!

# Misc Notes

* max7456 is the OSD I think -- text overlay for the video screen inside the head
* led matrices are all ws2812b
* The LED channels have min|max brightness limits (out of 255).
    * Face: 10|100 (max limit for power reasons)
    * Others: 30|255

# TODOs

"port the software"

* hook the LEDs up to the automatic brightness control
* LEDs are controlled from the animation files (face only?)
