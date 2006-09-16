pkgdatadir = $(datadir)/@PACKAGE@

gfx.zip: gfx/*
	zip gfx.zip gfx/*

gf2x.zip: gf2x/*
	zip gf2x.zip gf2x/*

sounds.zip: sounds/*
	zip sounds.zip sounds/*

scripts.zip: scripts/*
	zip scripts.zip scripts/*

zipfiles = gfx.zip gf2x.zip sounds.zip scripts.zip
pkgdata_DATA = $(zipfiles) config.xml inputconfig.xml

EXTRA_DIST = $(pkgdata_DATA)

DISTCLEANFILES = $(zipfiles)
