sw rsx 
deassign 
assign sy:[20,109] src 
assign sy:[20,11] bin 
crun mp src:self.c self.mp 
xcc self.mp 
pip self.mp/de 
xas -d self 
tkb self.tem=self,c:uem/lb,c:rstslb/lb,c:c/lb 
pip bin:self.tsk<232>=self.tem/w 
pip self.tem,self.obj/de/w 

