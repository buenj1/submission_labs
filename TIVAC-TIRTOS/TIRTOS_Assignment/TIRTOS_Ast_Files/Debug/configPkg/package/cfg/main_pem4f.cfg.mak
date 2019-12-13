# invoke SourceDir generated makefile for main.pem4f
main.pem4f: .libraries,main.pem4f
.libraries,main.pem4f: package/cfg/main_pem4f.xdl
	$(MAKE) -f C:\Users\jbuen\workspace_v9\TIVAC_TIRTOS_AST/src/makefile.libs

clean::
	$(MAKE) -f C:\Users\jbuen\workspace_v9\TIVAC_TIRTOS_AST/src/makefile.libs clean

