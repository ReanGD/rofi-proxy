#!/bin/sh

dir=`dirname "$(readlink -f "$0")"`
rofi -modi combi -show combi -combi-modi "proxy,drun" -proxy-log -proxy-cmd "$dir/sub_modes.py"
