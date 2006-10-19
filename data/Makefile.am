pkgdatadir = $(datadir)/@PACKAGE@

gfx.zip: gfx/*
	zip gfx.zip gfx/*.bmp backgrounds/*.bmp

gf2x.zip: gf2x/*
	zip gf2x.zip gf2x/*.bmp

sounds.zip: sounds/*
	zip sounds.zip sounds/*.wav

scripts.zip: scripts/*
	zip scripts.zip scripts/*.lua

zipfiles = gfx.zip gf2x.zip sounds.zip scripts.zip
pkgdata_DATA = $(zipfiles) config.xml inputconfig.xml

EXTRA_DIST = $(pkgdata_DATA)

DISTCLEANFILES = $(zipfiles)
