sw rsx 
deassign 
assign sy:[20,109] src 
assign sy:[20,11] bin 
crun mp src:mpost.c mpost.mp 
xcc mpost.mp 
pip mpost.mp/de 
xas -d mpost 
crun mp src:malib.c malib.mp 
xcc malib.mp 
pip malib.mp/de 
xas -d malib 
crun mp src:adpar.c adpar.mp 
xcc adpar.mp 
pip adpar.mp/de 
xas -d adpar 
tkb  
mpost.tem,mpost/wi=mpost,malib,adpar,c:uem/lb,c:rstslb/lb,c:c/lb 
/ 
stack=6000 
// 
pip bin:mpost.tsk<232>=mpost.tem/w 
pip mpost.tem,mpost.obj,malib.obj,adpar.obj/de/w 
ut ccl mpost= 
ut ccl mpost=bin:mpost.*;priv 30000 

