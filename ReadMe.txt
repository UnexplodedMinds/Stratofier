RoscoPi
AHRS and ADS-B Traffic display app for RaspberryPi
----------------------------------------------------------

(c) Allen K. Lair
    Unexploded Minds

Updated 2019-01-03

DO NOT RELY ON THIS APP FOR YOUR PRIMARY FLIGHT INSTRUMENTATION.

This app is meant solely as a backup and a convenient alternative
display of available traffic and attitude information available
from ADS-B through the Stratux ADS-B receiver with an AHRS daughter
board.

Most of the hard work is done by the Stratux itself and this app
is mostly a "dumb" display for that data.  It is not meant to replace
far more robust EFB apps.  It is meant to be used as a simple display
for the AHRS and available ADS-B data provided by the Stratux.  This
app has been modified from the Rosco Android app to run on a modest sized
screen connected directly to the Raspberry Pi HDMI port.

This app depends heavily on the Qt libraries, version 5.12.0 as of this
writing.

For build and install instructions, see INSTALL.txt

This software is provided for free under the GPL v3 license.

There are only a few controls:

The + and - buttons on the left side of the heading indicator zoom indicator
and out from 5 NM to 100 NM of airspace surrounding you, which is in the center
of the heading indicator.  The current airspace size is displayed in the bottom
left corner just above the G-Force indicator.

The Menu button has three options:

Reset Level:  Allows you to re-specify what is considered level in the Stratux
              software.
			  
Exit RoscoPi: Exit the RoscoPi display app back to the X-Windows desktop.

Shutdown:     Shuts down your Raspberry Pi.

The rest of the display should be self-explanatory.  It's mostly similar to
many other glass panel style all-in-one displays.  Obviously the speed displayed
is GPS-derived ground speed, NOT airspeed since the Stratux can't provide that
data without an external pitot-static sensor.

That's it.  All the data is supplied by the Stratux software.  You can also
use RoscoPi as a standalone display on another Raspberry Pi as long as it can
connect to the Stratux WiFi access point.

Enjoy!
