%define name vic
%define version 2.8ucl1.1.5
%define release ipv6

Name: %{name}
Summary: Video Conferencing Tool
Version: %{version}
Release: %{release}
Group: Applications/Internet
Copyright: Copyright (c) 1995-2003 University College London
URL: http://www-mice.cs.ucl.ac.uk/multimedia/software/%{name}/
Source: http://www-mice.cs.ucl.ac.uk/multimedia/software/%{name}/releases/%{version}/%{name}-%{version}.tar.gz
BuildRoot: %{_tmppath}/%{name}-root

%description
VIC is a real-time, multimedia application for video 
conferencing over the Internet.  It was developed by 
the Network Research Group at the Lawrence Berkeley 
National Laboratory in collaboration with the 
University of California, Berkeley.

See http://www-mice.cs.ucl.ac.uk/multimedia/software/vic

%prep
%setup -q

%build
cd tcl-8.0/unix
%configure
make
cd ../../tk-8.0/unix
%configure
make
cd ../../common
%configure --enable-ipv6
make
cd ../vic
%configure --sysconfdir=/etc --mandir=%{_mandir} --enable-ipv6
make

%install
rm -rf $RPM_BUILD_ROOT
install -d $RPM_BUILD_ROOT{/usr/bin,%{_mandir}/man1}
install -m755 -s vic/vic $RPM_BUILD_ROOT/usr/bin/vic
install -m644 vic/vic.1 $RPM_BUILD_ROOT%{_mandir}/man1/vic.1

%clean
rm -rf $RPM_BUILD_ROOT

%files 
%defattr(-,root,root)
%{_prefix}/bin/*
%{_mandir}/*/*

%changelog
* Fri Apr 25 2003 Kristian Hasler <k.hasler@cs.ucl.ac.uk>
- initial build

