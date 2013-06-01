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

proc run_resource_dialog { name email } {
	set w .form
	global V
	frame $w

	frame $w.msg -relief ridge
	label $w.msg.label -font [mediumfont] -wraplength 4i \
		-justify left -text \
"Please specify values for the following X resources. \
These strings will identify you by name and by email address \
in any RTP-based conference.  Please use your real name and \
affiliation instead of a ``handle'', e.g., ``Jane Doe (ACME Research)''. \
The values you enter will be saved in ~/.RTPdefaults so you will \
not have to re-enter them.  You can either leave this file in place \
or merge the contents with your normal X resources." -relief ridge

	pack $w.msg.label -padx 6 -pady 6
	pack $w.msg -side top

	foreach i {name email} {
		frame $w.$i -bd 2
		entry $w.$i.entry -relief sunken
		label $w.$i.label -width 10 -anchor e
		pack $w.$i.label -side left
		pack $w.$i.entry -side left -fill x -expand 1 -padx 8
	}
	$w.name.label config -text *rtpName:
	$w.email.label config -text *rtpEmail:
	pack $w.msg -pady 10
	pack $w.name $w.email -side top -fill x

	$w.$i.entry insert 0 [email_heursitic]

	frame $w.buttons
	button $w.buttons.accept -text Accept -command "set dialogDone 1"
	button $w.buttons.dismiss -text Quit -command "set dialogDone -1"
	pack $w.buttons.accept $w.buttons.dismiss \
		-side left -expand 1 -padx 20 -pady 10
	pack $w.buttons
	pack $w -padx 10

	global dialogDone
	while { 1 } {
		set dialogDone 0
		focus $w.name.entry
		tkwait variable dialogDone
		if { $dialogDone < 0 } {
			exit 0
		}
		set name [string trim [$w.name.entry get]]
		if { [string length $name] <= 3 } {
			open_dialog "please enter a reasonable name"
			continue
		}
		set email [string trim [$w.email.entry get]]
		if { [string first . $email] < 0 || \
			[string first @ $email] < 0 } {
			open_dialog "email address should have form user@host.domain"
			continue
		}
		break
	}
	option add *rtpName "$name" interactive
	option add *rtpEmail "$email" interactive

	set f [open ~/.RTPdefaults w+ 0644]
	puts $f "*rtpName: $name"
	puts $f "*rtpEmail: $email"
	close $f

	pack forget $w
	destroy $w
}
