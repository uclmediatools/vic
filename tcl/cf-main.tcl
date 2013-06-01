#
# Copyright (c) 1995 The Regents of the University of California.
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
# 1. Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
# 3. Neither the names of the copyright holders nor the names of its
#    contributors may be used to endorse or promote products derived from
#    this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS
# IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
# THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
# PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE
# LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
# CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
# SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
# INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
# CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
# ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.
#

set rtp_type(126) raw
set rtp_type(20) ldct
set rtp_type(21) pvh
set rtp_type(22) bvc
set rtp_type(25) cellb
set rtp_type(26) jpeg
set rtp_type(27) cuseeme
set rtp_type(28) nv
set rtp_type(29) picw
set rtp_type(30) cpv
set rtp_type(31) h261
set rtp_type(32) mpeg
set rtp_type(42) h263+
set rtp_type(34) h263
set rtp_type(127) h261v1
set rtp_type(77) h261as
set rtp_type(123) dv
set rtp_type(45) mpeg4
# RFC3984 compliant H.264
set rtp_type(96) h264  
# IOCOM's IG2 H.264 format
set rtp_type(107) h264_IOCOM


proc vic_main {} {
	global V tcl_platform

	set V(class) Vic    
	set V(app) vic
	set V(media) video
	init_local
	init_confbus
	init_network
	#
	# Set up log file
	#
	if [yesno logFrameRate] {
		global logFrameChannel
		set logFrameFile "vic-[clock seconds]-[gethostname]-[user_heuristic].log"
		set logFrameChannel [open "$logFrameFile" "w" ]
        puts $logFrameChannel "#\[src sdes cname] \[clock seconds] fps bps loss"
		#set logFrameChannel [open [resource logFrameFile] w ]
	}

	if [yesno relateInterface] {
		if {$tcl_platform(platform) == "windows"} {
			after 9000
		}
		build.new.interface
	} else {
		init_gui
	}
	init_late
	build.menu
    if [yesno transmitOnStartup] {
		global transmitButton
		$transmitButton invoke
	}

	set autoquit [option get . autoquit Vic]
	if { $autoquit != "" } {
		after $autoquit adios
	}
}
