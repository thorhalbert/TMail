sw rsx 
deassign 
assign sy:[20,109] src 
assign sy:[20,11] bin 
crun mp src:mail.c mail.mp 
!xcc -p mail.mp 
xcc mail.mp 
pip mail.mp/de 
xas -d mail 
crun mp src:malib.c malib.mp 
xcc malib.mp 
pip malib.mp/de 
xas -d malib 
tkb  
mail.tem,mail/wi=mail,malib,c:uem/lb,c:rstslb/lb,c:c/lb 
/ 
stack=6000 
// 
pip bin:mail.tsk<232>=mail.tem/w 
pip mail.tem,mail.obj,malib.obj/de/w 
pip bin:mail.edt<40>=src:mail.edt 
pip bin:mail.hlp<40>=src:mail.hlp 
pip bin:mailiv.hlp<40>=src:mailiv.hlp 
ut ccl mail-= 
ut ccl mail-=bin:mail.tsk;priv 30000 

