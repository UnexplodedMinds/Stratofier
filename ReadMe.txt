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

The Menu button has six options:

Reset Level:  Allows you to re-specify what is considered level in the Stratux
              software.

Reset G-Mtr:  Allows you to reset the G-force indicator the same as in the
	      stratux settings.

All Traffic/Cls Traffic:  Filter traffic showing all (unfiltered) or only traffic
			  that is within 5000 feet above or below you.
			  
Exit RoscoPi: Exit the RoscoPi display app back to the X-Windows desktop.

Shutdown:     Shuts down your Raspberry Pi.

Upgrade:      Upgrades the RoscoPi software.  You will see an LXTerminal
              window open showing compile progress.  Once it finishes it will
	      reboot and you should have the most recent version.  This pulls
	      the source code from github and compiles it.  You need an active
	      connection to the internet in order to do this.  The easiest way
	      to accomplish this is plug your ethernet port from your Pi
	      into your router or any available ethernet plug on your network.
	      Note also that this only upgrades the RoscoPi software, not the
	      Stratux software.  That must be done according to the instructions
	      from the stratux.me website.

Tapping the center of the heading indicator shows a short selection dialog for
Heading bug, Wind Bug, Clear or Cancel.  Selecting a wind bug will query for
both wind direction and speed.

Long pressing on the center of the heading indicator will display the crosswind
component if you have a heading and wind bug set.  It won't do anything if you
don't already have those set.

The rest of the display should be self-explanatory.  It's mostly similar to
many other glass panel style all-in-one displays.  Obviously the speed displayed
is GPS-derived ground speed, NOT airspeed since the Stratux can't provide that
data without an external pitot-static sensor.

That's it.  All the data is supplied by the Stratux software.  You can also
use RoscoPi as a standalone display on another Raspberry Pi as long as it can
connect to the Stratux WiFi access point.

Enjoy!
