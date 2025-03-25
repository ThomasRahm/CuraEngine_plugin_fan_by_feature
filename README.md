# Fan speed by feature type plugin

This plugin allows your fan speed to be different based on which line type (also called feature type) is printed. 

The following feature types can have a separate fan speed:
- Outer wall
- Inner wall
- Infill
- Top/Bottom
- Support
- Support interface
  
It also adds settings to handle that fans do not activate instantly, but need to spin up/down.


## How to install

[Download the latest .curapackage](https://github.com/ThomasRahm/CuraEngine_plugin_fan_by_feature/releases/download/1.0.0/CuraEngineFanSpeedByFeatureType_v1.0.0.curapackage), and drag and drop it into your Cura. You will need to restart Cura to finish the installation of this plugin.  
Please note that this plugin is intended for Cura 5.10+ and may cause crashes in earlier versions!

**You may need to adjust setting visibility to see the settings added by this plugin!**

## FAQ

Q: Why do I see no effect on the fan speed?  
A: Your minimum layer time settings may influence your fan speed, increasing the configured fan speed to your `Maximum Fan Speed` if a layer is printed too fast. Check your `Regular/Maximum Fan Speed Threshold` or **disable fan speed changes caused by your minimum layer time settings by changing `Minimum Layer Time Fan Mode` to `Never`.** If `Fan Velocity Change Time` makes your configured fan speed not reachable for a given feature type, the fan speed will instead be set to the value representing the lowest reachable fan speed for this feature type. **Setting `Fan Velocity Change Time` to 0** will disable this.

Q: This makes the noise of my printer much more annoying.  
A: Yes. There is no way for me to prevent fan noise from being influenced by fan speed :(

Q: Why would I use this?  
A: Part cooling is known to reduce layer adhesion, but improve surface quality. Depending on the printed feature type one may prefer one over the other. Note that you need to configure the fan speeds yourself, as the default value for each feature type fan speed is the normal fan speed!
