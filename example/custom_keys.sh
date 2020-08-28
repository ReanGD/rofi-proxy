#!/bin/sh

dir=`dirname "$(readlink -f "$0")"`
rofi -kb-custom-1 "Alt+1" -kb-custom-2 "Alt+2" -modi proxy -show proxy -proxy-log -proxy-cmd "$dir/custom_keys.py"
