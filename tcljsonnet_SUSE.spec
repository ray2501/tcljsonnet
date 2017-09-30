%{!?directory:%define directory /usr}

%define buildroot %{_tmppath}/%{name}

Name:          tcljsonnet
Summary:       Jsonnet wrapper for Tcl
Version:       0.8
Release:       1
License:       Apache License, Version 2.0
Group:         Development/Libraries/Tcl
Source:        https://sites.google.com/site/ray2501/tcljsonnet/tcljsonnet_0.8.zip
URL:           https://sites.google.com/site/ray2501/tcljsonnet
BuildRequires:	autoconf
BuildRequires:	automake
BuildRequires:	gcc-c++
BuildRequires:	libstdc++-devel
BuildRequires: tcl-devel >= 8.4
Requires: tcl >= 8.4
BuildRoot:     %{buildroot}

%description
Jsonnet wrapper for Tcl.

Jsonnet is a domain specific configuration language that helps you 
define JSON data. Jsonnet lets you compute fragments of JSON within 
the structure, bringing the same benefit to structured data that 
templating languages bring to plain text.

%prep
%setup -q -n %{name}

%build
export CC=g++
CFLAGS="%optflags" ./configure \
	--prefix=%{directory} \
	--exec-prefix=%{directory} \
	--libdir=%{directory}/%{_lib}
make 

%install
make DESTDIR=%{buildroot} pkglibdir=%{directory}/%{_lib}/tcl/%{name}%{version} install

%clean
rm -rf %buildroot

%files
%defattr(-,root,root)
%{directory}/%{_lib}/tcl
