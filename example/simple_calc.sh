#!/bin/sh

dir=`dirname "$(readlink -f "$0")"`
rofi -modi proxy -show proxy -proxy-log -proxy-cmd "$dir/simple_calc.py"
