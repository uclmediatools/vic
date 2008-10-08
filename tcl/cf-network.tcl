#
# Copyright (c) 1995 Regents of the University of California.
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
# 3. All advertising materials mentioning features or use of this software
#    must display the following acknowledgement:
#	This product includes software developed by the Network Research
#	Group at Lawrence Berkeley National Laboratory.
# 4. Neither the name of the University nor of the Laboratory may be used
#    to endorse or promote products derived from this software without
#    specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
# ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
# FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
# DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
# OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
# HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
# LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
# OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
# SUCH DAMAGE.
#

# $dst has form addr/port/fmt/ttl (last two optional)
#
proc net_open_ip { sessionType session dst } {
	global V numLayers IPaddrFamily

        set layer 0
	set c $V(class)
	set dst [split $dst /]
	set n [llength $dst]
	if { $n < 2 } {
		warn "must specify both address and port in the form addr/port"
		exit 1
	} else {
		set addr [lindex $dst 0]
		set port [lindex $dst 1]
		if { ![string match \[0-9\]* $port] || $port >= 65536 } {
			warn "illegal port '$port'"
			exit 1
		}
		if { $n >= 3 } {
			if { $sessionType == "vat" } {
				set confid [lindex $dst 2]
				if { $n >= 4 } {
					set fmt [lindex $dst 3]
					if { $n >= 5 } {
						set ttl [lindex $dst 4]
						if { $n > 5 } {
							usage
						}
					}
				}
			} else {
				set fmt [lindex $dst 2]
				if { $n >= 4 } {
					set ttl [lindex $dst 3]
					if { $n > 4 } {
						usage
					}
				}
			}
		}
		if { [info exists fmt] && $fmt != "" } {
			option add $c.defaultFormat $fmt interactive
			#XXX compat with vat
			option add $c.audioFormat $fmt interactive
		}	
		if [info exists confid] {
			option add $c.confid $confid interactive
		}	
		if [info exists ttl] {
			option add $c.defaultTTL $ttl interactive
		}	
	}
	set ttl [resource defaultTTL]
	if { $ttl < 0 || $ttl > 255 } {
		warn "invalid ttl ($ttl)"
		exit 1
	}
	if { $sessionType == "rtp" } {
		set port [expr $port &~ 1]
	}
	set dn [new network $IPaddrFamily]
	if { $dn == "" } {
		warn "Not compiled with $IPaddrFamily support"
		exit 1
	}
	if { [$dn open $addr $port $ttl] == 0 } {
		warn "Problem opening $IPaddrFamily data socket"
		exit 1
	}
	$session data-net $dn
	if { $sessionType != "nv" } {
		if { $sessionType == "ivs" } {
			incr port 2
		} else {
			incr port
		}
		set cn [new network $IPaddrFamily]
		$cn open $addr $port $ttl
		$session ctrl-net $cn
		set V(ctrl-net) $cn
	}
	set V(data-net) $dn

	if { $numLayers > 0 } {
	    if { $IPaddrFamily == "ip6" } {
		    set base [string range [$dn addr] 0 [string last : [$dn addr]]]
		    set offset [string range [$dn addr] [expr {[string last : [$dn addr]]+1}] end]
		    set separator :
	    } else {
		    set oct [split [$dn addr] .]
		    set base [lindex $oct 0].[lindex $oct 1].[lindex $oct 2]
		    set offset [lindex $oct 3]
		    set separator .
	    }
        		
            while { $numLayers > $layer } {
			incr port 
			incr layer
			if { [$dn ismulticast] } { incr offset }
			set ldn [new network $IPaddrFamily]
			$ldn open $base$separator$offset $port $ttl
			$session data-net $ldn $layer

			if { $sessionType != "nv" } {
				if { $sessionType == "ivs" } {
					incr port 2
				} else {
					incr port
				}
				set cn [new network $IPaddrFamily]
				$cn open $base$separator$offset $port $ttl
				$session ctrl-net $cn $layer
			}
            }
	}

	#
	# if the max bandwidth wasn't set, pick one based on the
	# session ttl.  Then use the max bandwidth to compute the
	# fraction allocated to rtcp (control) traffic.  We have
	# to do this before creating the session manager since it
	# will use the result to compute a time to the first report.
	# bw is in kbits/sec and we want bytes/ms so we divide by 8.
	#
	set maxbw [resource maxbw]
	if { $maxbw < 0 } {
		if { $V(app) == "vat" } {
			# maxbandwidth wasn't set - pick one based on
			# the data format assuming the format was picked
			# to match some bottleneck bandwidth.
			switch [resource audioFormat] {
				pcm  { set maxbw 78 }
				pcm2 { set maxbw 71 }
				pcm4 { set maxbw 68 }
				dvi  { set maxbw 46 }
				dvi2 { set maxbw 39 }
				dvi4 { set maxbw 36 }
				gsm  { set maxbw 17 }
				lpc4 { set maxbw 9 }
				default { set maxbw 71 }
			}
		} else {
			if { $ttl <= 16 || ![$dn ismulticast] } {
				set maxbw 3072
			} elseif { $ttl <= 64 } {
				set maxbw 1024
			} elseif  { $ttl <= 128 } {
				set maxbw 128
			} elseif { $ttl <= 192 } {
				set maxbw 53
			} else {
				set maxbw 32
			}
		}
		option add $c.maxbw $maxbw interactive
	}
}

#
# $dst is an "atm-host"
#
proc net_open_atm { sessionType session dst } {
	global V

	set dataSAP 4092
	set ctrlSAP 4090
	set dn [new network atm]
	if { $dn == "" } {
		warn "not compiled with ATM support"
		exit 1
	}
	#XXX should be able to configure qos parameters
	if { [$dn open $dst $dataSAP 256 64 128 128 2 1] < 0 } {
		warn "cannot open atm connection to $dst"
		exit 1
	}
	$session data-net $dn
	set cn [new network atm]
	if { [$cn open $dst $ctrlSAP 128 64 24 24 2 1] < 0 } {
		warn "cannot open atm control connection to $dst"
		exit 1
	}
	$session ctrl-net $cn
	set V(ctrl-net) $cn
	set V(data-net) $dn
}

#
# $dst has form ip-addr/rtip-port
#
proc net_open_rtip { sessionType session dst } {
	global V

	set dst [split $dst /]
	set n [llength $dst]
	if { $n != 2 } {
		warn  "must specify both address and port (i.e., addr/port) for RTIP"
		exit 1
	}
	set addr [lindex $dst 0]
	set port [lindex $dst 1]
	set dn [new network rtip]
	if { [$dn open $addr $port [resource rtipXmin] [resource rtipXave] \
		[resource rtipI] [resource rtipSmax] [resource rtipD] \
		[resource rtipJ] [resource rtipZ] [resource rtipW] \
		[resource rtipU]  [resource rtipType]] < 0 } {
		warn "cannot open RTIP connection to $addr/$port"
		exit 1
	}
	# just send control over data port
	$session data-net $dn
	$session ctrl-net $dn
	set V(ctrl-net) $cn
	set V(data-net) $dn
}

proc init_network {} {
	global numLayers numEncoderLayers IPaddrFamily
	
	set numLayers [resource numLayers]
	set numEncoderLayers $numLayers
	set netType [resource network]
	set IPaddrFamily [resource network]

	# Auto detect IPv6 addresses
	if { [string first ":" [resource defaultHostSpec]] > 0 } {
		set IPaddrFamily ip6
	}
	# Check for ipv6 as well as ip6 as a netType
	if { $IPaddrFamily == "ipv6" } {
		set IPaddrFamily ip6
	}
	if { [info procs net_open_$netType] == "" } {
		warn "$netType not a recognized network type"
		exit 1
	}
	set sessionType [resource sessionType]
	if { $sessionType == "rtpv2" || $sessionType == "vic" } {
		set sessionType rtp
	}
	global V
	set V(sessionType) $sessionType
	set V(session) [new session $V(media)/$sessionType]
	if { $V(session) == "" } {
		warn "'$sessionType' not a recognized session type"
		exit 1
	}
	$V(session) mtu [resource mtu]
	net_open_$netType $sessionType $V(session) [resource defaultHostSpec]


	$V(session) max-bandwidth [resource maxbw]
	$V(session) lip-sync [yesno lipSync]

	set key [resource sessionKey]
	if { $key != "" } {
		crypt_set $key
	}
}

proc crypt_format {key sessionType} {
	if {$sessionType == "vat"} {
		set fmt VAT
	} else {
		set k [string first / $key]
		if { $k < 0 } {
			set fmt DES1
		} else {
			set fmt [string range $key 0 [expr $k - 1]]
			set key [string range $key [expr $k + 1] end]
		}
	}
	return "$fmt $key"
}

proc crypt_set key {
	global doEncryption entryTab V
	set L [crypt_format $key $V(sessionType)]
	set fmt [lindex $L 0]
	set key [lindex $L 1]
	set dc data-crypt:$fmt
	set cc ctrl-crypt:$fmt
	if ![info exists V($dc)] {
		set crypt [new crypt $fmt/data]
		if { $crypt == "" } {
			open_dialog "no $fmt enryption support"
			return -1
		}
		set V($dc) $crypt
		set V($cc) [new crypt $fmt/ctrl]
	}
	if [$V($dc) key $key] {
		$V($cc) key $key
		$V(data-net) crypt $V($dc)
		$V(ctrl-net) crypt $V($cc)
		set V(encrypt) 1
		return 0
	} else {
		open_dialog "your key is cryptographically weak"
		crypt_clear
		return -1
	}
}

proc crypt_clear {} {
	global V
	$V(data-net) crypt ""
	$V(ctrl-net) crypt ""
	set V(encrypt) 0
}
