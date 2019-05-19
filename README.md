# alsavolctl
Python package to monitor and control ALSA volume on my media player RaspberryPI

Implemented so far:
* vol_webui. Intented for use with https://github.com/gthar/vol_webui. The script should communicate via WebSockets to that static website.

TODO:
* Hardware volume view (probably with a 7-segment display)
* Hardware volume control (probably with a digital knob)

This is a fairly naive package that makes many assumptions that probably won't hold true outside of my use case.
