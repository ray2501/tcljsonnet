# -*- tcl -*-
# Tcl package index file, version 1.1
#
if {[package vsatisfies [package provide Tcl] 9.0-]} {
    package ifneeded jsonnet 0.17 \
	    [list load [file join $dir libtcl9jsonnet0.17.so] [string totitle jsonnet]]
} else {
    package ifneeded jsonnet 0.17 \
	    [list load [file join $dir libjsonnet0.17.so] [string totitle jsonnet]]
}
