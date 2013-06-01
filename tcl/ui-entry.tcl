#
# Copyright (c) 1993-1994 The Regents of the University of California.
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
# For any entry created, the global (array) entryTab($w:action)
# contains the tcl procedure name to invoke when the entry is
# updated (where $w is the wind path of the entry widget).
# This proc is passed two args -- the window and the new value.
# It should return 0 if the edit should be accepted,
# and non-zero otherwise.  entryTab($w:value) should be
# set to the initial value when the widget is created.
#

bind Entry <Enter> {
	catch { 
		global entryTab
		if { $entryTab(%W:focus) } {
			focus %W
		}
	}
}
bind Entry <Return> {
	focus .
	%W select clear
	catch {
		global entryTab
		set entryTab(%W:focus) 0
		set v [%W get]
		if { [$entryTab(%W:action) %W $v] } {
			%W delete 0 end
			%W insert 0 $entryTab(%W:value)
		} else {
			set entryTab(%W:value) $v
		}
	}
}
bind Entry <Escape> {
	focus .
	%W select clear
	%W delete 0 end
	catch {
		global entryTab
		set entryTab(%W:focus) 0
		%W insert 0 $entryTab(%W:value)
	}
}
bind Entry <Control-g> [bind Entry <Escape>]

bind Entry <Control-u> {
	tkEntrySetCursor %W 0
	%W delete insert end
}
