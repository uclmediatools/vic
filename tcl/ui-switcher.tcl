#
# Copyright (c) 1993-1996 The Regents of the University of California.
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

#
# This module handles the video switcher abstraction.  Generic objects
# are registered as swithable with a method to call back when switching
# occurs.  By using a generic callback mechanism, this code is shared
# between the on screen windows and external video outputs.
# 

proc next_active_src src {
	set list [session active]
	set k [lsearch -exact $list $src]
	# (if we didn't find the list element, k will be -1 and incr'd to 0)
	incr k
	if { $k >= [llength $list] } {
		set k 0
	}
	return [lindex $list $k]
}

proc prev_active_src src {
	set list [session active]
	set k [lsearch -exact $list $src]
	if { $k < 0 } {
		set k 0
	} else {
		if { $k == 0 } {
			set k [llength $list]
		}
		incr k -1
	}
	return [lindex $list $k]
}

proc switcher_register { o src method } {
	global switcher_src switcher_method
	set switcher_src($o) $src
	set switcher_method($o) $method
}

proc switcher_unregister o {
	switcher_cancel_timer $o
}

proc switcher_enable o {
	switcher_touch $o
}

proc switcher_disable o {
	global switcher_ts
	unset switcher_ts($o)
}

proc switcher_set_timer o {
	switcher_sched $o
}

proc switcher_cancel_timer o {
	global switcher_timer_id
	if [info exists switcher_timer_id($o)] {
		after cancel $switcher_timer_id($o)
		unset switcher_timer_id($o)
	}
}

proc switcher_set { o src } {
	global switcher_method switcher_src
	if { $src != $switcher_src($o) } {
		$switcher_method($o) $o $src
		set switcher_src($o) $src
	}
}

proc switcher_next o {
	global switcher_src
	switcher_set $o [next_active_src $switcher_src($o)]
}

proc switcher_prev o {
	global switcher_src
	switcher_set $o [prev_active_src $switcher_src($o)]
}

set switcher_clock 1

proc switcher_touch o {
	global switcher_clock switcher_ts
	set switcher_ts($o) $switcher_clock
	incr switcher_clock
}

set cb_dispatch(focus) cb_switcher
proc cb_switcher msg {
	foreach s [session active] {
		#
		# look up the site by cname.  (for backward compat
		# with old vat's, try the IP address too)
		#
		if { [$s addr] == $msg || [$s sdes cname] == $msg } {
			switcher_focus $s
			return
		}
	}
	# if we've received a message and it's not local then default to low bw anyway
	if { [yesno bw_control] } {
		set_bps .menu.cb.frame.right.bps.value 32
	}
}

proc switcher_focus src {
	global switcher_ts switcher_method

	# if this is a local source then bump up the frame rate otherwise default to low bw
	if { [yesno bw_control] } {
		if {"[srctab local]" == $src } {
			set_bps .menu.cb.frame.right.bps.value [.menu.cb.frame.right.bps.scale get]
		} else {
			set_bps .menu.cb.frame.right.bps.value 32
		}
	}

	if ![info exists switcher_ts] {
		return
	}
	set target ""
	foreach o [array names switcher_ts] {
		if { $target == "" || \
			$switcher_ts($o) < $switcher_ts($target) } {
			set target $o
		}
	}
	if { $target != "" && \
		( "[srctab local]" != $src || [yesno vain] ) } {
		switcher_set $target $src
		switcher_touch $target
	}
}

proc switcher_sched o {
	global switcher_timer_id
	set ms [expr 1000 * [option get . switchInterval Vic]]
	set switcher_timer_id($o) [after $ms "switcher_timer $o"]
}

proc switcher_timer o {
	global switcher_timer_id
	if [info exists switcher_timer_id($o)] {
		switcher_next $o
		switcher_sched $o
	}
}

proc is_switched o {
	global switcher_ts switcher_is_timed
	return [expr [info exists switcher_ts($o)] || \
			[info exists switcher_timer_id($o)]]
}
