sw rsx 
deassign 
assign sy:[20,109] src 
assign sy:[20,11] bin 
xcc src:mailtb.c 
xas -d mailtb 
tkb mailtb.tem=mailtb,c:uem/lb,c:c/lb 
pip bin:mailtb.tsk<104>=mailtb.tem/w 
pip mailtb.tem,mailtb.obj/de/w 

