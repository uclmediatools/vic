#!/bin/sh
# $Id$

SCALE=100

cat trace.r | grep now | awk '{print $4" "$6" "$8}' > xr_send.tr
cc/tools/xr_send xr_send xr_send.tr $SCALE


