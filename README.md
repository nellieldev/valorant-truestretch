# Valorant Truestretch
## Preconfiguration (in Valorant)
- Open your video settings in Valorant options
- Set your aspect ratio method to "Fill"
- Set your window mode to "Windowed" (**not FULLSCREEN or anything else**)
- It is recommended to keep your default resolution in-game before using this tool

## Configuration
- Create a config file in the same directory named `config.ini` with following contents:
```ini
[ValorantTrueStretch]
screen_index=1
aspect_ratio=4:3
```
- Set the `screen_index` to the index of the screen where the Valorant window is located (index starts with 0 and ends with the number of your screens minus one)
- Set the `ascept_ratio` to the aspect ratio you want your game to be stretched to (most likely `4:3`) - The program will find the best resolution itself

## Finalizing
- Run `valorant-truestretch.exe` with the `config.ini` in the same directory
- Start Valorant
- Your screen resolution should be updated upon the Valorant loading screen shows up