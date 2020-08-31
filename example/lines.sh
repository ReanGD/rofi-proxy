#!/bin/sh

dir=`dirname "$(readlink -f "$0")"`
rofi -show-icons -modi proxy -show proxy -proxy-log -proxy-cmd "$dir/lines.py"
