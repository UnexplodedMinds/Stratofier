#!/bin/bash

# Update the sources and build
zenity --progress --text="Upgrading RoscoPi. Please wait..." --percentage=0 --auto-close
cd /home/pi/RoscoPi
zenity --progress --text="Upgrading RoscoPi. Please wait..." --percentage=33 --auto-close
git pull
zenity --progress --text="Upgrading RoscoPi. Please wait..." --percentage=66 --auto-close
make
zenity --progress --text="Upgrading RoscoPi. Please wait..." --percentage=100 --auto-close
bin/RoscoPi
