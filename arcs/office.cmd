sw rsx 
deassign 
assign sy:[20,109] src 
assign sy:[20,11] bin 
crun mp src:office.c office.mp 
crun mp src:sndmes.c sndmes.mp 
crun mp src:malib.c malib.mp 
!xcc -p office.mp 
xcc office.mp 
pip office.mp/de 
xcc sndmes.mp 
pip sndmes.mp/de 
xcc malib.mp 
pip malib.mp/de 
xas -d office 
xas -d sndmes 
xas -d malib 
tkb office.tem=office,sndmes,malib,c:uem/lb,c:rstslb/lb,c:c/lb 
pip bin:office.tsk<104>=office.tem/w 
pip office.tem,office.obj,sndmes.obj,malib.obj/de/w 

