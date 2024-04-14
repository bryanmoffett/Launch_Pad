# Launch_Pad
Arduino software to control a launch pad for a thrust-vector-controlled model rocket. The launch pad uses Sparkfun Qwiic components for lights, sound, clamp control and igniting the rocket motor fuse.

The system uses and Redboard Artemis, a Qwiic LED strip, two Qwiic Alphanumeric displays, a Qwiic relay, and a Qwiic buzzer. There's also a 20kg servo to control the launch ring clamps, although I do not use that any longer as it's one potential failure during a launch, and if it's windy enough you need the clamps, you shouldn't be launching a TVC rocket. There's also a physical key-based lockout for safety, in the relay circuit.

This code is for educational purposes. Use or modify at your own risk. High-powered rockets are dangerous, and I make no claims that this code is safe enough for others to use.
