# batflash2
Shutter controller for photographing bats using the HAMA CA-1 remote control

This is software for an Arduino contraption I made for a friend who is photographing bats.

It works as follows:
* a bat flies into the infrared-beam of a 3rd party photographic flash trigger system
* the flash fires, exposing the image of the bat onto a camera
* my contraption detects the flash using a kind of slave trigger photocell
* my contraption sends a wireless signal to the camera to end the exposure (and save the picture)
* my contraption sends a wireless signal to the camera to start a new exposure ("bulb mode")
* the cycle repeats, waiting for another bat to trigger a flash
Additionally, every X seconds without any activity, a new exposure is started anyway.

The wireless signal is a 433 MHz AM modulated radio signal compatible with Hama CA-1 remote controls.
The Arduino emulates the Hama CA-1 radio transmitter using a generic 433 MHz on-off module.
The Hama CA-1 radio receiver is connected to the camera.

