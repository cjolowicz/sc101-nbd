#!/usr/bin/perl
package PSAN;
use IO::Socket;
use IO::Select;
use strict;

sub find
{
    my @ret;

    my $sock = IO::Socket::INET->new(
	Proto     => 'udp',
	Broadcast => 1,
    ) or die "Can't bind: $@\n";

    my $broadcast = sockaddr_in(20_001, INADDR_BROADCAST);

    $sock->send("\x0d\x00\x00\x00", 0, $broadcast);

    while (IO::Select->new($sock)->can_read(1))
    {
	my $addr = $sock->recv(my $reply, 65536);

	push @ret, PSAN::Disk->new($reply)
	    if substr($reply, 0, 2) eq "\x0e\x00";
    }

    @ret;
}

package PSAN::Disk;
use Socket;
use Math::BigInt;
use Encode qw/decode/;
use strict;

sub new
{
    my $class = ref $_[0] ? ref shift : shift;
    my $packet = shift;

    return bless {
	_addr => inet_ntoa(substr $packet, -4),
    }, $class;
}

sub _info
{
    my $self = shift;

    return $self->{_info} if exists $self->{_info};

    my $sock = IO::Socket::INET->new(
	Proto     => 'udp',
	PeerAddr  => $self->addr,
	PeerPort  => 20_001,
    ) or die "Can't bind: $@\n";

    $sock->send("\x00\x09\x00\x00".("\x00"x18).pack("N", 0)."\x00\x00", 0);

    IO::Select->new($sock)->can_read(1)
	or return;

    $sock->recv($self->{_info}, 65536);

    return $self->{_info};
}

sub addr { shift->{_addr} }
sub version { unpack "Z*" => substr(shift->_info(), 28, 16) }
sub total_space { Math::BigInt->new("0x" . unpack "H*" => substr(shift->_info(), 52, 6))->bmul(512) }
sub free_space { Math::BigInt->new("0x" . unpack "H*" => substr(shift->_info(), 58, 6))->bmul(512) }
sub partitions {
    my $self = shift;
    my $count = unpack "C" => substr($self->_info(), 69, 1);
    @{ $self->{_partitions} ||= [ map PSAN::Partition->new($self->addr, $_), 1..$count ] }
}
sub label { decode("UTF-16", "\xff\xfe".substr(shift->_info(), 74, 56)) }

package PSAN::Partition;
use Socket;
use strict;

sub new
{
    my $class = ref $_[0] ? ref shift : shift;
    my ($root_addr, $partition) = @_;

    return bless {
	_root_addr => $root_addr,
	_partition => $partition,
    }, $class;
}

sub _info
{
    my $self = shift;

    return $self->{_info} if exists $self->{_info};

    my $sock = IO::Socket::INET->new(
	Proto     => 'udp',
	PeerAddr  => $self->root_addr,
	PeerPort  => 20_001,
    ) or die "Can't bind: $@\n";

if ($self->partition) {
    $sock->send("\x00\x09\x00\x00".("\x00"x18).pack("N", $self->partition)."\x00\x00", 0);
} else {
    $sock->send("\x13\x00\x00\x00".("\x00"x24), 0);
}

    IO::Select->new($sock)->can_read(1)
	or return;

    $sock->recv($self->{_info}, 65536);

    return $self->{_info};
}

sub _nameres
{
    my $self = shift;

    return $self->{_addr} if exists $self->{_addr};

    my $sock = IO::Socket::INET->new(
	Proto     => 'udp',
	Broadcast => 1,
    ) or die "Can't bind: $@\n";

    my $broadcast = sockaddr_in(20_001, INADDR_BROADCAST);

    $sock->send("\x0f\x00\x00\x00".pack("Z64" => $self->id), 0, $broadcast);

    IO::Select->new($sock)->can_read(1)
	or return;

    my $addr = $sock->recv(my $reply, 65536);

    return $self->{_addr} = inet_ntoa(substr($reply, 80, 4));
}

sub root_addr { shift->{_root_addr} }
sub partition { shift->{_partition} }
sub addr { shift->_nameres }
sub label { unpack "Z*" => substr(shift->_info(), 34, 28) }
sub id { unpack "Z*" => substr(shift->_info(), 206, 64) }
sub size { Math::BigInt->new("0x" . unpack "H*" => substr(shift->_info(), 162, 6))->bmul(512) }

package main;
use strict;

foreach my $disk (PSAN->find)
{
    printf "%s\n", "="x132;

    printf "Version: %-16s\tRoot IP Addr: %s\n", $disk->version, $disk->addr;
    printf "Total(MB): %-10s\t\t# Partitions: %s\n", $disk->total_space/1024/1024, scalar $disk->partitions;
    printf "Free(MB): %-10s\n", $disk->free_space/1024/1024;

    printf "%s\n", "-"x132;
    foreach my $part ($disk->partitions)
    {
	printf "%-40s %-28s %-15s %s\n",
	    $part->id,
	    $part->label,
	    $part->addr,
	    $part->size/1024/1024;
    }
}
