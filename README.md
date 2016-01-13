tcljsonnet
=====

Jsonnet is a domain specific configuration language that helps you 
define JSON data. Jsonnet lets you compute fragments of JSON within 
the structure, bringing the same benefit to structured data that 
templating languages bring to plain text.

For additional information on Jsonnet see

[http://jsonnet.org] (http://jsonnet.org)


License
=====

Apache License, Version 2.0


UNIX BUILD
=====

Building under most UNIX systems is easy, just run the configure script
and then run make. For more information about the build process, see
the tcl/unix/README file in the Tcl src dist. The following minimal
example will install the extension in the /opt/tcl directory.

	$ cd tcljsonnet
	$ export CC=g++
	$ ./configure --prefix=/opt/tcl
	$ make
	$ make install
	
If you need setup directory containing tcl configuration (tclConfig.sh),
below is an example:

	$ cd tcljsonnet
	$ export CC=g++
	$ ./configure --with-tcl=/opt/activetcl/lib
	$ make
	$ make install


WINDOWS BUILD
=====

The recommended method to build extensions under windows is to use the
Msys + Mingw build process. This provides a Unix-style build while
generating native Windows binaries. Using the Msys + Mingw build tools
means that you can use the same configure script as per the Unix build
to create a Makefile.


Implement commands
=====

jsonnet::evaluateFile - accepts a file name string.  
jsonnet::evaluateSnippet - accepts a string to evaluate. 


Example
=====

    package require jsonnet

    set snippet {// example_operators.jsonnet
    {
        foo: [1, 2, 3],
        bar: [x * x for x in self.foo if x >= 2],
        baz: { ["field" + x]: x for x in self.foo },
        obj: { ["foo" + "bar"]: 3 },
    }}

    jsonnet::evaluateSnippet $snippet

