#
# Copyright (c) 1993-1995 The Regents of the University of California.
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

global font
if {$::tk_version < 8.5} {
	if {[string match [ windowingsystem] "aqua"]} {
	        font create medfont -family {Lucida Grande} -size 12 -weight bold
	        font create smallfont -family {Lucida Grande} -size 10 -weight bold
	        font create minifont -family {Lucida Grande} -size 4
	        font create helpfont -family {Lucida Grande} -size 12
	        font create entryfont -family {Lucida Grande} -size 10
	} else { 
	        set font(helvetica10) {
	        r-normal--*-100-75-75-*-*-*-*
	        r-normal--10-*-*-*-*-*-*-*
	        r-normal--11-*-*-*-*-*-*-*
	        r-normal--*-100-*-*-*-*-*-*
	        r-normal--*-*-*-*-*-*-*-*
	    }
	        set font(helvetica12) {
	        r-normal--*-120-75-75-*-*-*-*
	        r-normal--12-*-*-*-*-*-*-*
	        r-normal--14-*-*-*-*-*-*-*
	        r-normal--*-120-*-*-*-*-*-*
	        r-normal--*-*-*-*-*-*-*-*
	    }
	        set font(helvetica14) {
	        r-normal--*-140-75-75-*-*-*-*
	        r-normal--14-*-*-*-*-*-*-*
	        r-normal--*-140-*-*-*-*-*-*
	        r-normal--*-*-*-*-*-*-*-*
	    }
	        set font(times14) {
	        r-normal--*-140-75-75-*-*-*-*
	        r-normal--14-*-*-*-*-*-*-*
	        r-normal--*-140-*-*-*-*-*-*
	        r-normal--*-*-*-*-*-*-*-*
	    }
	}
}

proc search_font { foundry style weight points } {
	global font
	foreach f $font($style$points) {
		set fname -$foundry-$style-$weight-$f
		if [havefont $fname] {
			return $fname
		}
	}
	puts stderr "vic: can't find $weight $fname font (using fixed)"
	if ![havefont fixed] {
		puts stderr "vic: can't find fixed font"
		exit 1
	}
	return fixed
}

proc init_fonts {} {
	global tcl_platform

	if {$::tk_version < 8.5} {
		set foundry [option get . foundry Vic]

		if {$tcl_platform(platform) == "windows"} {
			set helv10  [search_font $foundry helvetica medium 12]
			set helv4b [search_font $foundry helvetica bold 10]
			set helv10b [search_font $foundry helvetica bold 12]
			set helv12b [search_font $foundry helvetica bold 12]
			set times14 [search_font $foundry times medium 14]
		} else {
			set helv4b [search_font $foundry helvetica bold 10]
			set helv10  [search_font $foundry helvetica medium 10]
			set helv10b [search_font $foundry helvetica bold 10]
			set helv12b [search_font $foundry helvetica bold 12]
			set times14 [search_font $foundry times medium 14]
		}

		option add *Font $helv12b startupFile
		option add Vic.medfont $helv12b startupFile
		option add Vic.smallfont $helv10b startupFile
		option add Vic.minifont $helv4b startupFile
		option add Vic.helpfont $times14 startupFile
		option add Vic.entryfont $helv10 startupFile
	} else {
		option add Vic.medfont TkDefaultFont
		option add Vic.smallfont TkSmallCaptionFont
		option add Vic.minifont TkIconFont
		option add Vic.helpfont TkTooltipFont
		option add Vic.entryfont TkTextFont
	}

    }

proc init_resources {} {

	global tcl_platform
	#
	# use 2 pixels of padding by default, except with MacOSX Aqua
	#
	if {![string match [ windowingsystem] "aqua"]} {
	    option add *video*padX 0
	    option add *video*padY 0
	    option add *padX 1
	    option add *padY 1
	}
	#
	# don't put tearoffs in pull-down menus
	#
	option add *tearOff 0

	#
	# Make the color scheme a little darker than the default gray.
	# Go through all the palette resources and raise the option data
	# base priority from widgetDefault to 61 so that user's X resources
	# won't override these.
	#
	if {$tcl_platform(platform) != "windows" && $::tk_version < 8.5} {
		tk_setPalette gray80
		foreach pal [array names tkPalette] {
			option add *$pal $tkPalette($pal) 61
		}
	} elseif {[windowingsystem] == "aqua" && $::tk_version > 8.4} {
		tk_setPalette systemSheetBackground
	}

	option add *Radiobutton.relief flat startupFile

	#
	# These can be overridden.
	#
	option add Vic.geometry 400x300 startupFile
	option add Vic.mtu 1024 startupFile
	option add Vic.network ip startupFile
	option add Vic.framerate 8 startupFile
	option add Vic.defaultTTL 16 startupFile
	option add Vic.maxbw -1 startupFile
	option add Vic.bandwidth 128 startupFile
	option add Vic.iconPrefix vic: startupFile
	option add Vic.priority 10 startupFile
	option add Vic.confBusChannel 0 startupFile

	option add Vic.defaultFormat h.261 startupFile
	option add Vic.sessionType rtpv2 startupFile
	option add Vic.grabber none startupFile
	option add Vic.stampInterval 1000 startupFile
	option add Vic.switchInterval 5 startupFile
	option add Vic.dither od startupFile
	option add Vic.tile 1 startupFile
	option add Vic.filterGain 0.25 startupFile
	option add Vic.statsFilter 0.0625 startupFile
	option add Vic.useHardwareDecode false startupFile
	option add Vic.infoHighlightColor LightYellow2 startupFile
	option add Vic.useJPEGforH261 false startupFile
	option add Vic.useHardwareComp false startupFile
	option add Vic.stillGrabber false startupFile 
	option add Vic.fileGrabber false startupFile 
	option add Vic.siteDropTime "300" startupFile
	option add Vic.quality "0" startupFile
	option add Vic.inputType "ntsc" startupFile

	option add Vic.brightness "128" startupFile
	option add Vic.contrast "128" startupFile
	option add Vic.chromau "0" startupFile
	option add Vic.chromav "0" startupFile
	option add Vic.saturation "230" startupFile
	option add Vic.hue "128" startupFile

	option add Vic.chroma_saturation "0" startupFile
	option add Vic.chroma_gain "0" startupFile
	option add Vic.luma_brightness "0" startupFile
	option add Vic.luma_contrast "0" startupFile

	option add Vic.antiflicker "disabled" startupFile
	option add Vic.v4l2gain "128" startupFile
	option add Vic.v4l2gamma "128" startupFile

	#
	# color resources
	#
	option add Vic.medianCutColors 150 startupFile
	option add Vic.gamma 0.7 startupFile

	option add Vic.yuv_byteOrder "0" startupFile

	#XXX
	option add Vic.jvColors 32 startupFile

	#
	# RTIP defaults
	#
	option add Vic.rtipXmin 655 startupFile
	option add Vic.rtipXave 655 startupFile
	option add Vic.rtipI 6553 startupFile
	option add Vic.rtipSmax 1200 startupFile
	option add Vic.rtipD 1200 startupFile
	option add Vic.rtipJ 3279 startupFile
	option add Vic.rtipZ 10000 startupFile
	option add Vic.rtipW 1000 startupFile
	option add Vic.rtipU 1000 startupFile
	option add Vic.rtipType 1 startupFile

	# Flow Label info for IPV6

	option add Vic.flowLabel 0 startupFile

	# Interface Index to bind to in IPv6
	option add Vic.ifIndex -1 startupFile

	# Interface addr to bind to and use in CNAME
	option add Vic.ifAddr 0 startupFile

	option add Vic.numLayers 0 startupFile

	option add Vic.foundry adobe startupFile

	# Lip sync option
	option add Vic.lipSync false startupFile

	# Overlay image
	option add Vic.overlayFilename "" startupFile
	option add Vic.overlayX "0" startupFile
	option add Vic.overlayY "0" startupFile

	# Init fonts
        if {![string match [ windowingsystem] "aqua"]} {
	    init_fonts
	}

	option add Vic.suppressUserName true startupFile

	option add Vic.softJPEGthresh -1 startupFile
	option add Vic.softJPEGcthresh 6 startupFile

	option add Vic.sunvideoDevice 0 startupFile
	option add Vic.enableBVC true startupFile

	# true to allow voice-switch to switch to self
	option add Vic.vain false startupFile

	# list of sdes items to display in info window
	option add Vic.sdesList "cname tool email note"

	# Set rtpNote for sender - This can be overidden by rtpNote in ~/.RTPdefaults
	option add Vic.rtpNote "" startupFile

	catch "option readfile ~/.RTPdefaults startupFile"
}
