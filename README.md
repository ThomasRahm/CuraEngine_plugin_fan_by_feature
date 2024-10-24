# CuraEngine_plugin_fan_by_feature

This plugin allows your fan speed to be different based on which type of line is printed. 

The following features can have a separate fan speed: Outer wall, inner wall, infill, top/bottom, support, support interface.
It also adds settings to handle that fans do not activate instantly, but need to spin up/down.


## How to install

Download the latest .curapackage, and drag and drop it into your Cura.
It currently only is available for x86 bases Windows and Linux.
It the future I plan to also release this on the Cura marketplace, but I did not yet have time for this.


## FAQ

Q: It seems not to be working! 
A: Your minimum layer time settings may influence your fan speed, increasing the configured fan speed to your `Maximum Fan speed` if a layer is printed too fast. Check your `Regular/Maximum Fan Speed Threshold` or disable fan speed changes caused by your minimum layer time settings by changing `Minimum Layer Time Fan Mode` to `Never`
