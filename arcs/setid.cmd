sw rsx 
deassign 
assign sy:[20,109] src 
assign sy:[20,11] bin 
cru mp src:setid.c setid.mp 
xcc setid.mp 
pip setid.mp/de 
xas -d setid 
tkb  
setid.tem=setid,c:rstslb/lb,c:c/lb 
/ 
stack=6000 
// 
pip bin:setid.tsk<104>=setid.tem/w 
pip setid.tem,setid.obj/de/w 

