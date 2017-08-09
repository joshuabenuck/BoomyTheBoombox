# BoomyTheBoombox
A customized sketch to run on a Boomy the Boombox build. Mine is done using the parts I got in AdaBox 004.

_This is very much a work in progress! My goal is to make this usable by others so if you give it a try and run into any problems, feel free to submit an issue!_

## Pre-reqs

* SdFat: Used instead of Arduino's SD library in order to get long filename support!
* ESP8266WiFi
* PubSubClient: Used to connect to MQTT broker

## Setup

On the root of your SD card create the following files:

* ssid: The SSID for your wifi network
* password: The unencrypted password to your wifi network
* broker: The IP address of your MQTT broker

Note: None of these files can have leading or trailing spaces or newlines. The parsing code isn't robust enough to handle it yet.

The button mappings for the remote are stored in *mappings*. The format is one path per line from 0-9 with newline line endings.

A future update will detect the absence of these files and will provide a user friendly way to initialize them.

## FAQ

When boomy successfully connects to the WiFi network, an audible beep will be played. At this point MQTT messages may be sent to it.

#### How do I add a new track without removing the SD card?

On one terminal run this to see the responses from boomy:

    mosquitto_sub -t boomy -t mp3s -t tracks -t track 

On another run these commands to list the current tracks, upload source.mp3 as dest.mp3, add dest.mp3 as the first track, play the first track by simulating a button press, and finally saving the tracks list so it persists across reboots.

    mosquitto_pub -t boomy -m /tracks
    curl -v -F "data=@source.mp3;filename=dest.mp3" boomy.local/edit
    mosquitto_pub -t boomy -m /track/1/dest.mp3
    mosquitto_pub -t boomy -m /press/1
    mosquitto_pub -t boomy -m /tracks/save

This will eventually be done via a UI of some sort, but this at least shows it is now possible.

## MQTT messages

Boomy subscribes to the _boomy_ topic to receive commands. These are the commands supported.

* /list: List all of the files on the SD card.
* /list/*path*: List all of the files under the given path.
* /tracks: Prints all of the currently configured tracks.
* /track/[0-9]: Returns the MP3 file which will play when the specified button is preseed.
* /track/[0-9]/*path*: Sets the MP3 at *path* to play when the button is pressed.
* /tracks/save: Saves all of the current track configurations.
* /press/[0-9]: Simulates a press of the corresponding button.
* /vol-: Reduces the volume.
* /vol+: Increases the volume.
* /volmax: Sets the volume to the maximum level.

## Startup Process

The sketch will do the following upon boot:

* Initialize the SD card
* Load the WiFi configuration
* Load the button mappings
* Connect to the WiFi (no timeout!)
* Play a beep to let you know the device is initialized
* Attempt to connect to the broker if there is no connection (done once per loop)
* Listen for IR or MQTT commands

After the beep has been played, you can either publish a control message to the _boomy_ topic on the configured MQTT broker or can use the IR remote to play the configured tracks.

## Improvements Over Original

This is based off of the excellent tutorial code by Adafruit and the examples from the various open source libraries used in the sketch.

Compared to the original Adafruit code, the following improvements have been made.

* Long filename support! No more 8.3 filename restrictions!
* WiFi connection information is stored on the filesystem and not in the sketch which makes committing the same sketch you use for development easier.
* MQTT control of the player. This lays the foundation for controlling it using AdafruitIO or If This Then That (ITTT). It also opens some interesting automation capabilities such as playing a song when some other external event is triggered.

## Missing Features

Not all of the features in the original are currently implemented. Here are the missing ones.

* No internet streaming support. Only local MP3s are supported at the moment.

## Rough Edges

* No easy configuration upon initial boot.
* Very little error handling in the parsing code.
* No stand alone support. The device must be able to connect to a WiFi network.
* No file upload or download support. The SD card must still be removed to add new MP3 files.
* Topic and message names, structure, and format are arbitrary and inconsistent. (Can anyone point me to some best practices in this area).
* No ability to support more than one boomy on a network given the topic and message name choices.
* With all of the network processing which occurs each loop, it is not uncommon to hear audio glitches multiple times during a given song. A fix for this may be to do the network processing less frequently or just to prioritize feeding the MP3 chip over everything else.

## Future direction

* Web UI to control the player and configure the button mappings.

