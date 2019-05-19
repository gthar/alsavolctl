# alsavolctl
Python package to monitor and control ALSA volume on my media player RaspberryPi.

## Installation
* Make sure the user that will run this exists: `useradd -M -G audio -s /usr/sbin/nologin volume`
* Make sure the expected virtual environment exists with Python3.7: `/usr/local/opt/virtualenvs/volume_ctl`
* `cp vol_webui.service /etc/systemd/system`
* `systemctl start vol_webui.service`
* `systemctl enable vol_webui.service`

Implemented so far:
* vol_webui. Intented for use with https://github.com/gthar/vol_webui. The script should communicate via WebSockets to that static website.

TODO:
* Hardware volume view (probably with a 7-segment display)
* Hardware volume control (probably with a digital knob)

---

Noe: This is a fairly naive package that makes many assumptions that probably won't hold true outside of my use case.
