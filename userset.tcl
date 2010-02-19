# userset.tcl ---
# Filename: userset.tcl
# Author: Soo-Hyun Choi <S.Choi@cs.ucl.ac.uk>
# Description: 
#        (1) "still" grabber will be enabled on startup
#        (2) "set dev" button will set device as "still"
#            ,and will load "foreman.yuv" file
#        (3) "tx now" will set bps/fps to max value,
#            ,and start transmitting
#
# Created: Feb. 19, 2010
# $Id$

option add Vic.stillGrabber true startupFile
option add Vic.device "still" startupFile

proc user_fps {val} {
	global fps_slider
	$fps_slider set $val
}

proc user_bps {val} {
	global bps_slider
	$bps_slider set $val
}

proc user_rates {} {
	global transmitButtonState transmitButton
	user_bps 3072
	user_fps 30
#	$transmitButton invoke
}

proc user_dev {} {
	global videoDevice inputDeviceList inputPort
	if { ![info exists env(VIC_DEVICE)] } {
		set deviceName "still"

		foreach v $inputDeviceList {
			puts "deviceList: [$v nickname]"
			if { [string last $deviceName [$v nickname]] != -1 } {
				set videoDevice $v
				select_device $v
				$v file "foreman.yuv"
				return
			}
		}
	}
}

proc user_tx {} {
	global transmitButton
	user_rates
	#user_dev
	$transmitButton invoke
}

proc user_hook {} {
	global videoDevice inputDeviceList inputPort
	global transmitButton transmitButtonState
	global logoButton logoButtonState

	frame .b -borderwidth 1
	pack .b -fill x
	button .b.dev -text "set dev" -command user_dev
	button .b.tx -text "tx now" -command user_tx
	pack .b.dev .b.tx -side right -padx 0p -pady 0
}
