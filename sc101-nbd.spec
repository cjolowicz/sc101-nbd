Name:           sc101-nbd
Version:        @pkg_version@
Release:        1
Summary:        SC101 NBD server

Group:          System/Servers
License:        GPL
URL:            http://code.google.com/p/sc101-nbd/
Source0:        sc101-nbd_@pkg_version@.tar.gz
BuildRoot:      %{_tmppath}/%{name}-%{version}-%{release}-root-%(%{__id_u} -n)

%description
This software allows Linux to access the the sc101 as a block device, without
requiring a new kernel module.

It achieves this by translating between the kernel's existing NBD protocol
and the reverse engineered protocol used by the sc101 (I call it PSAN, perhaps
incorrectly).

On top of the NBD devices you can run RAID, LVM, and any existing filesystems
(ext2/3, ocfs2, etc).


%prep
%setup -q
# we want to install in /sbin, not /usr/sbin...
%define _exec_prefix %{nil}


%build
make


%install
rm -rf $RPM_BUILD_ROOT
make install DESTDIR=$RPM_BUILD_ROOT


%clean
rm -rf $RPM_BUILD_ROOT


%post
chkconfig --add ut


%files
%defattr(-,root,root,-)
%doc
%config(noreplace) %{_sysconfdir}/uttab
%{_sysconfdir}/init.d/ut
%{_sbindir}/ut
%{_mandir}/man8/ut.8*


%changelog
* Tue May 29 2007 - Iain Wade <iwade@optusnet.com.au>
- initial build
