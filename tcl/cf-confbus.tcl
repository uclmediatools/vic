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

proc confbusHandler { cb msg } {
	global cb_dispatch
	if { [llength $msg] < 1 } {
		debug "bad confbus message: $msg"
		return
	}
	set class [lindex $msg 0]
	if ![info exists cb_dispatch($class)] {
		debug "no such confbus method: $class"
		return
	}
	set proc $cb_dispatch($class)
	set formals [info args $proc]
	set actuals [lrange $msg 1 end]
	#if { [llength $actuals] != [llength $formals] } {
	#	debug "confbus arg mismatch: $class ($formals)/($actuals)"
	#	return		
	#}

	debug "\nproc=$proc, actuals=$actuals, formals=$formals, cb_dispatch=$cb_dispatch($class), msg=$msg"

        if [catch "$proc $actuals"] {
                debug "$V(app): something wrong while executing \"proc actuals\""
                exit 1
        }

	#debug "DEBUG confbusHandler: after proc actuals"
}

proc init_confbus {} {
	set channel [resource confBusChannel]

	debug "DEBUG init_confbus{}: initialising confBus"

	if { $channel != 0 } {
		global V
		set V(cb) [new confbus $channel]
		$V(cb) handler confbusHandler
	}
}

#XXX
#proc cb_announce {} {
#	global V
#	$V(cb) send "pulse video"
#}
