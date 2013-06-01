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

#
# Handle commands that come over conference bus for manipulating
# overlaid images (e.g., from a title generator).
#

proc tm_bootstrap {} {
	global tm420 tm422
	if ![info exists tm420] {
		set tm420 [new module compositor/420]
		set tm422 [new module compositor/422]
	}
}

proc tm_init { grabber encoder } {
	global tmEnable
	if ![info exists tmEnable] {
		return 0
	}
	set ff [$encoder frame-format]
	if { $ff == "cif" } {
		set ff 420
	}
	if { $ff == "420" || $ff == "422" } {
		global tm420 tm422
		tm_bootstrap
		set tm [set tm$ff]
		$tm target $encoder
		$grabber target $tm
		return 1
	} else {
		return 0
	}
}

proc tm_enable {} {
	global tmEnable V

	if [info exists tmEnable] {
		return
	}
	set tmEnable 1
	if [have grabber] {
		tm_init $V(grabber) $V(encoder)
	}
}

proc tm_disable {} {
	global tmEnable V

	catch "unset tmEnable"
	if [have grabber] {
		# remove the title-maker from the path
		# XXX this assumes the title-maker is the only
		# intervening modules
		$V(grabber) target $V(encoder)
	}
}

proc tm_check id {
	global tm_obj
	return [info exists tm_obj($id)]
}

proc tm_create { id file width height } {
puts create/$id/$file/$width/$height

	global tm_obj
	if [tm_check $id] {
		tm_destroy $id
	}
	set tm_obj($id) [new overlay]
	#XXX check for error
	$tm_obj($id) load $file $width $height
	tm_bootstrap
}

proc tm_transparent { id lum } {
puts transparent:$id/$lum
	global tm_obj
	if [tm_check $id] {
		set o $tm_obj($id)
		$o transparent $lum
	}
}

proc tm_destroy id {
	global tm420 tm422 tm_obj
	if [tm_check $id] {
		set o $tm_obj($id)
		unset tm_obj($id)
		$tm420 detach $o
		$tm422 detach $o
		delete $o
	}
}

proc tm_place { id x y depth } {
	global tm420 tm422 tm_obj tm_depth
	if [tm_check $id] {
		set o $tm_obj($id)
		if { ![info exists tm_depth($o)] } {
			set tm_depth($o) $depth
			$tm420 attach $o $x $y $depth
			$tm422 attach $o $x $y $depth
		} elseif { $tm_depth($o) != $depth } {
			set tm_depth($o) $depth
			$tm420 detach $o
			$tm422 detach $o
			$tm420 attach $o $x $y $depth
			$tm422 attach $o $x $y $depth
		} else {
			$tm420 move $o $x $y
			$tm422 move $o $x $y
		}
	}
}

proc tm_remove id {
	global tm420 tm422 tm_obj
	if [tm_check $id] {
		set o $tm_obj($id)
		unset tm_obj($id)
		$tm420 detach $o
		$tm422 detach $o
		delete $o
	}
}

#
# initialize the dispatch table with the title-maker API
#
foreach proc { tm_enable tm_disable tm_create tm_destroy \
	tm_place tm_remove tm_transparent } {
	set cb_dispatch($proc) $proc
}
