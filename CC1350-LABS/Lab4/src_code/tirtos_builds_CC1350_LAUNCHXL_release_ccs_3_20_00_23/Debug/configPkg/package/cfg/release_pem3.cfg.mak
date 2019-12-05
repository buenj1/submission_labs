# invoke SourceDir generated makefile for release.pem3
release.pem3: .libraries,release.pem3
.libraries,release.pem3: package/cfg/release_pem3.xdl
	$(MAKE) -f C:\Users\jbuen\workspace_v9\tirtos_builds_CC1350_LAUNCHXL_release_ccs_3_20_00_23/src/makefile.libs

clean::
	$(MAKE) -f C:\Users\jbuen\workspace_v9\tirtos_builds_CC1350_LAUNCHXL_release_ccs_3_20_00_23/src/makefile.libs clean

