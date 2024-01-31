# Dungeon Camera
The app was created to improve my online D&D sessions and make them more immersive.

![Screenshot](screenshot.png)

What it basically does is takes input from your camera, draws overlay with character stats, linek HP, AC, level etc. and outputs it to vistual camera output.
The stats are loaded from D&D Beyond.

## Build
Requires [libuv](https://github.com/libuv/libuv) to build.

## Run
Requires [VCamSDK](https://www.e2esoft.com/sdk/vcam-sdk/) to run.

Before fisrt start you need to install VCamSDK. To do that open win-dshow folder and run `virtualcam-install.bat`.
You can change image which will be shown if no stream started by replaceing `placeholder.png`.
Run `dungeon-camera.exe`. Enter you caracter Id on D&D Beyond and click **Reload**.
In your videochat change video input from your camera to **Dungeon Camera**.
