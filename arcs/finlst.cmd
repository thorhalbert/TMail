sw rsx 
deassign 
assign sy:[20,109] src 
assign sy:[20,11] bin 
xcc src:finlst.c 
xas -d finlst 
tkb finlst.tem=finlst,c:uem/lb,c:c/lb 
pip bin:finlst.tsk<104>=finlst.tem/w 
pip finlst.tem,finlst.obj/de/w 

