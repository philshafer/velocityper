#
# Demo for JUNOS
#
-w 20ms -b 70ms -l 200ms
-P "ready to start demo"
configure private
edit protocols bgp group foo neighbor 1.2.3.4
-P "bgp complete
set apply-macro foo one 1
set apply-macro foo two 2
show
-P "apply complete"
set apply-lock user phil
up 1
protect neighbor 1.2.3.4
show
-P "protect complete
show | compare
-P "done"





