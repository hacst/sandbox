# sandbox

## What is it?
A OpenCV based augmented reality sandbox implementation. This repository contains the code created by our three person team during this 5 day university project. The basic idea is to sense the height map of a sand surface the user can interact with in the real world and project a corresponding elevation map of the surface right back onto it.

While there are a lot of better implementations of this concept already out there we nevertheless are quite pleased about what we could achieve in 5 days without any previous knowledge of either OpenCV nor image processing. We even had some time left over on the last day to implement a treasure hunting Easter egg in which you can dig for a hidden treasure chest. All in all this was way to much fun ;) Playing with sand all day and getting credits for it.

The code has not been touched after the projects conclusion so expect rough edges.

## Pictures
Unfortunately the images don't really do justice to how vivid the colors look when projected onto the white sand but here are some anyways:


Sandbox with a few mountains and valleys to show off the colorband:
![Sandbox with a few mountains and valleys to show off the colorband](https://github.com/dD0T/sandbox/raw/master/media/Sandbox3.jpg)

Another picture with the same colorband:
![Another picture with the same colorband](https://github.com/dD0T/sandbox/raw/master/media/Sandbox4.jpg)

Getting creative with a different colorband:
![Getting creative with a different colorband](https://github.com/dD0T/sandbox/raw/master/media/Sandbox5.jpg)

Yet another example with different colors:
![Yet another example with different colors.](https://github.com/dD0T/sandbox/raw/master/media/Sandbox6.jpg)

And another one:
![And another one](https://github.com/dD0T/sandbox/raw/master/media/Sandbox7.jpg)

Picture of the cropped and undistorted B/W depth field picked up by the sensor.
![Picture of the cropped and undistorted B/W depth field picked up by the sensor](https://github.com/dD0T/sandbox/raw/master/media/Sandbox2.jpg)

Picture of the sandbox setup in the lab:
![Picture of the sandbox setup in the lab](https://github.com/dD0T/sandbox/raw/master/media/Sandbox1.jpg)

Animation of the auto-calibration method which detects the extent of the projection area as well as distortion. It does so by projecting circles into the edges in a specific order:
![Animation of the auto-calibration methods which detects the extent of the projection area as well as distortion by projecting circles into the edges in a specific .order](https://github.com/dD0T/sandbox/raw/master/media/AutoCalibrationCircles.gif)


## Requirements

### Hardware
* ASUS XTion PRO Live
	* In theory any OpenCV/OpenNI compatible sensor like Kinect might do with a bit of work
* Beamer
	* Ours had a 1024x768 resolution
* Box with white sand
	* We had approximately 9cm of sand
* Some shovels to play with the sand

### Software
* OpenCV 2.4.0
	* Needs OpenNI support compiled in
* Visual Studio 2010
* Git

## Authors
In no specific order:
* Aleksej Lednev
* Patrick Hillert <<silent@gmx.biz>>
* Stefan Hacker <<mail@hacst.net>>
    