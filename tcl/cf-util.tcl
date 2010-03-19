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

proc rtp_format src {
	global rtp_type
	set fmt [$src format]
	if [info exists rtp_type($fmt)] {
		return $rtp_type($fmt)
	} elseif { $fmt < 0 }  {
		return ""
	} else {
		return fmt-$fmt
	}
}

proc local_fqdn {} {
	set host [gethostname]
	if { [string first . $host] < 0 } {
		# not a fully qualified domain name
		return ""
	}
	return $host
}

proc init_local {} {
	global V
	set f ~/.$V(app).tcl
	if [file exists $f] {
		uplevel #0 "source $f"
	}
	set script [resource startupScript]
	if { $script != "" } {
		uplevel #0 "source $script"
	}
	#
	# make sure we have mandatory resources
	#
	set name [resource rtpName]

	if { $name == "" } {
		set name [resource sessionName]
		option add *rtpName $name startupFile
	}
	set email [resource rtpEmail]
	if { $name == "" || $email == "" } {
		run_resource_dialog $name $email
	}

}

proc email_heursitic {} {
	set user [user_heuristic]
	set addr [local_fqdn]
	if { $addr == "" } {
		return ""
	}
	return $user@$addr
}

proc user_heuristic {} {
	global env
	if [info exists env(USER)] {
		set user $env(USER)
	} elseif [info exists env(LOGNAME)] {
		set user $env(LOGNAME)
	} else {
		#
		# try vic built-in which is present under windows
		#
		catch "set env(USER) [getusername]"
		if [info exists env(USER)] {
			return $env(USER)
		}
		return "UNKNOWN"
	}
}

# default user_hook is a nop -- users should re-define this in their
# ~/.{vic,vat}.tcl to (re-)define procedures to change or augment
# the tools behavior.

proc user_hook {} {
}

proc init_late {} {
	global V env
	
	# create the local rtp-source object
	set net $V(data-net)
	
	# choose the initial RTP srcid
	set srcid [session random-srcid [$net addr]]

	# create the local source object
	set src [srctab create-local $srcid [$net interface] $net]
	
	set cname [resource cname]
	if { $cname == "" } {
		set interface [$net interface]
		if { $interface == "0.0.0.0" } {
			# this happens under solaris
			set interface [session local-addr-heuristic]
		}
		set cname [user_heuristic]@$interface
	}

	$src sdes name [resource rtpName]
	$src sdes email [resource rtpEmail]
	$src sdes note [resource rtpNote]
	$src sdes cname $cname
	$src sdes loc [resource loc]
	set tool $V(app)-[version]
	global tcl_platform
	if {[info exists tcl_platform(os)] && $tcl_platform(os) != "" && \
	    $tcl_platform(os) != "unix"} {
		set p $tcl_platform(os)
		if {$tcl_platform(osVersion) != ""} {
			set p $p-$tcl_platform(osVersion)
		}
		if {$tcl_platform(machine) != ""} {
			set p $p-$tcl_platform(machine)
		}
		set tool "$tool/$p"
	}
	$src sdes tool $tool

	# set the window & icon title from the conference name
	global prefix
	set prefix [resource iconPrefix]
	set conf [resource conferenceName]
	if { $conf == "" } {
		global V
		set net $V(data-net)
		set conf [$net addr]/[$net port]
	}
	wm iconname . "$prefix$conf"
	wm title . $conf
	set V(confname) [wm title .]

	#XXX these don't go here
	set V(useHardwareDecode) [yesno useHardwareDecode]
	srctab site-drop-time [resource siteDropTime]

	user_hook
}

proc warn s {
	global V
	puts stderr "$V(app): $s"
}

proc debug s {
	if [yesno debug] {
		warn $s
	}
}

proc have what {
	global V
	return [info exists V($what)]
}

proc resource r {
	global V
	return [option get . $r $V(class)]
}

proc yesno attr {
	set v [resource $attr]
	if { [string match \[0-9\]* $v] } {
		return $v
	}
	if { $v == "true" || $v == "True" || $v == "t" } {
		return 1
	}
	return 0
}

#
# Return the best name for the given source, based on
# the possibly limited information we have.
#
proc getid src {
	set name [$src sdes name]
	if { $name == "" } {
		set name [$src sdes cname]
		if { $name == "" } {
			set name [$src addr]
		}
	}
	return $name
}
