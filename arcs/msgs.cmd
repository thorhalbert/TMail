sw rsx 
deassign 
assign sy:[20,109] src 
assign sy:[20,11] bin 
xcc src:msgs.c 
xas -d msgs 
tkb msgs.tem=msgs,c:uem/lb,c:c/lb 
pip bin:msgs.tsk<104>=msgs.tem/w 
pip msgs.tem,msgs.obj/de/w 

