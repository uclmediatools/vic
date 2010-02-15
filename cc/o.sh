#!/bin/sh

# packet seqno trace in time
grep seqno trace.s | grep -v pid | awk '{if($1=="now:" && $3=="seqno:") print}' | awk '{print $2" "$4}' > seqno.tr
cc/tools/asv seqno.tr
rm seqno.tr

# start encoding
grep h261_encode_start trace.s | grep -v pid | awk '{print $3" "100}' > encs.xg

# end encoding
grep h261_encode_end trace.s | grep -v pid | awk '{print $3" "100}' > ence.xg

# encoding time difference
grep enc_time trace.s | grep -v pid | awk '{if($1=="num:" && $3=="enc_time:") print}' | awk '{print $2"\t"$4}' > enct.xg

# start grab
grep start_grab trace.s | awk '{print $3" "100}' > grs.xg

# end grab
grep end_grab trace.s | awk '{print $3" "100}' > gre.xg

# grabbing time difference
grep grab_time trace.s | awk '{print $2"\t"$4}' > grt.xg

# incoming XR
grep incomingXR trace.s | awk '{print $3" "100}' > inXR.xg
