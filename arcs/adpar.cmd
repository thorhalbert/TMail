pip adpar.obj,malib.obj/de:no 
assign sy:[20,109] src 
xcc src:malib 
assign sy: src 
xcc src:adpar 
xas -d adpar 
xas -d malib 
tkb adpar=adpar,malib,c:c/lb 
pip adpar.obj,malib.obj/de:no 

