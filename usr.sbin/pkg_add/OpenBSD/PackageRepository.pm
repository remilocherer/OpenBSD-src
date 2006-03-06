# ex:ts=8 sw=4:
# $OpenBSD: PackageRepository.pm,v 1.2 2006/03/06 10:40:31 espie Exp $
#
# Copyright (c) 2003-2004 Marc Espie <espie@openbsd.org>
#
# Permission to use, copy, modify, and distribute this software for any
# purpose with or without fee is hereby granted, provided that the above
# copyright notice and this permission notice appear in all copies.
#
# THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
# WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
# MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
# ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
# WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
# ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
# OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.

use strict;
use warnings;

package OpenBSD::PackageRepository;
use OpenBSD::PackageLocation;

sub _new
{
	my ($class, $address) = @_;
	bless { baseurl => $address }, $class;
}

sub new
{
	my ($class, $baseurl) = @_;
	if ($baseurl =~ m/^ftp\:/i) {
		return OpenBSD::PackageRepository::FTP->_new($baseurl);
	} elsif ($baseurl =~ m/^http\:/i) {
		return OpenBSD::PackageRepository::HTTP->_new($baseurl);
	} elsif ($baseurl =~ m/^scp\:/i) {
		require OpenBSD::PackageRepositorySCP;

		return OpenBSD::PackageRepository::SCP->_new($baseurl);
	} elsif ($baseurl =~ m/src\:/i) {
		require OpenBSD::PackageRepository::Source;

		return OpenBSD::PackageRepository::Source->_new($baseurl);
	} else {
		return OpenBSD::PackageRepository::Local->_new($baseurl);
	}
}

sub available
{
	my $self = shift;

	return @{$self->list()};
}

sub wipe_info
{
	my ($self, $pkg) = @_;

	require File::Path;

	my $dir = $pkg->{dir};
	if (defined $dir) {

	    File::Path::rmtree($dir);
	    delete $pkg->{dir};
	}
}

# by default, all objects may exist
sub may_exist
{
	return 1;
}

# by default, we don't track opened files for this key

sub opened
{
	undef;
}

# hint: 0 premature close, 1 real error. undef, normal !

sub close
{
	my ($self, $object, $hint) = @_;
	close($object->{fh}) if defined $object->{fh};
	$self->parse_problems($object->{errors}, $hint) 
	    if defined $object->{errors};
	undef $object->{errors};
	$object->deref();
}

sub finish_and_close
{
	my ($self, $object) = @_;
	$self->close($object);
}

sub close_now
{
	my ($self, $object) = @_;
	$self->close($object, 0);
}

sub close_after_error
{
	my ($self, $object) = @_;
	$self->close($object, 1);
}

sub close_with_client_error
{
	my ($self, $object) = @_;
	$self->close($object, 1);
}

sub make_room
{
	my $self = shift;

	# kill old files if too many
	my $already = $self->opened();
	if (defined $already) {
		# gc old objects
		if (@$already >= $self->maxcount()) {
			@$already = grep { defined $_->{fh} } @$already;
		}
		while (@$already >= $self->maxcount()) {
			my $o = shift @$already;
			$self->close_now($o);
		}
	}
	return $already;
}

# open method that tracks opened files per-host.
sub open
{
	my ($self, $object) = @_;

	return undef unless $self->may_exist($object->{name});

	# kill old files if too many
	my $already = $self->make_room();
	my $fh = $self->open_pipe($object);
	if (!defined $fh) {
		return undef;
	}
	$object->{fh} = $fh;
	if (defined $already) {
		push @$already, $object;
	}
	return $fh;
}

sub find
{
	my ($repository, $name, $arch, $srcpath) = @_;
	$name.=".tgz" unless $name =~ m/\.tgz$/;
	my $self = OpenBSD::PackageLocation->new($repository, $name);

	return $self->openPackage($name, $arch);
}

sub grabPlist
{
	my ($repository, $name, $arch, $code) = @_;
	$name.=".tgz" unless $name =~ m/\.tgz$/;
	my $self = OpenBSD::PackageLocation->new($repository, $name);

	return $self->grabPlist($name, $arch, $code);
}

sub parse_problems
{
	my ($self, $filename, $hint) = @_;
	CORE::open(my $fh, '<', $filename) or return;

	my $baseurl = $self->{baseurl};
	local $_;
	my $notyet = 1;
	while(<$fh>) {
		next if m/^(?:200|220|221|226|229|230|227|250|331|500|150)[\s\-]/;
		next if m/^EPSV command not understood/;
		next if m/^Trying [\da-f\.\:]+\.\.\./;
		next if m/^Requesting \Q$baseurl\E/;
		next if m/^Remote system type is\s+/;
		next if m/^Connected to\s+/;
		next if m/^remote\:\s+/;
		next if m/^Using binary mode to transfer files/;
		next if m/^Retrieving\s+/;
		next if m/^Succesfully retrieved file/;
		next if m/^\d+\s+bytes\s+received\s+in/;
		next if m/^ftp: connect to address.*: No route to host/;

		if (defined $hint && $hint == 0) {
			next if m/^ftp: -: short write/;
			next if m/^421\s+/;
		}
		if ($notyet) {
			print STDERR "Error from $baseurl:\n" if $notyet;
			$notyet = 0;
		}
		if (m/^421\s+/ ||
		    m/^ftp: connect: Connection timed out/ ||
		    m/^ftp: Can't connect or login to host/) {
			$self->{lasterror} = 421;
		}
		if (m/^550\s+/) {
			$self->{lasterror} = 550;
		}
		print STDERR  $_;
	}
	CORE::close($fh);
	unlink $filename;
}

package OpenBSD::PackageRepository::Installed;
our @ISA=qw(OpenBSD::PackageRepository);
use OpenBSD::PackageInfo;

sub new
{
	bless {}, shift;
}

sub find
{
	my ($repository, $name, $arch, $srcpath) = @_;
	my $self;

	if (is_installed($name)) {
		$self = OpenBSD::PackageLocation->new($repository, $name);
		$self->{dir} = installed_info($name);
	}
	return $self;
}

sub grabPlist
{
	my ($repository, $name, $arch, $code) = @_;
	require OpenBSD::PackingList;
	return  OpenBSD::PackingList->from_installation($name, $code);
}

sub available
{
	return installed_packages();
}

sub list
{
	my @list = installed_packages();
	return \@list;
}

sub wipe_info
{
}

sub may_exist
{
	my ($self, $name) = @_;
	return is_installed($name);
}

package PackageRepository::Source;

sub find
{
	my ($repository, $name, $arch, $srcpath) = @_;
	my $dir;
	my $make;
	if (defined $ENV{'MAKE'}) {
		$make = $ENV{'MAKE'};
	} else {
		$make = '/usr/bin/make';
	}
	if (defined $repository->{baseurl} && $repository->{baseurl} ne '') {
		$dir = $repository->{baseurl}
	} elsif (defined $ENV{PORTSDIR}) {
		$dir = $ENV{PORTSDIR};
	} else {
		$dir = '/usr/ports';
	}
	# figure out the repository name and the pkgname
	my $pkgfile = `cd $dir && SUBDIR=$srcpath ECHO_MSG=: $make show=PKGFILE`;
	chomp $pkgfile;
	if (! -f $pkgfile) {
		system "cd $dir && SUBDIR=$srcpath $make package BULK=Yes";
	}
	if (! -f $pkgfile) {
		return undef;
	}
	$pkgfile =~ m|(.*/)([^/]*)|;
	my ($base, $fname) = ($1, $2);

	my $repo = OpenBSD::PackageRepository::Local->_new($base);
	return $repo->find($fname);
}

package OpenBSD::PackageRepository::Local;
our @ISA=qw(OpenBSD::PackageRepository);

sub open_pipe
{
	my ($self, $object) = @_;
	my $pid = open(my $fh, "-|");
	if (!defined $pid) {
		die "Cannot fork: $!";
	}
	if ($pid) {
		return $fh;
	} else {
		open STDERR, ">/dev/null";
		exec {"/usr/bin/gzip"} 
		    "gzip", 
		    "-d", 
		    "-c", 
		    "-q", 
		    "-f", 
		    $self->{baseurl}.$object->{name}
		or die "Can't run gzip";
	}
}

sub may_exist
{
	my ($self, $name) = @_;
	return -r $self->{baseurl}.$name;
}

sub list
{
	my $self = shift;
	my $l = [];
	my $dname = $self->{baseurl};
	opendir(my $dir, $dname) or return $l;
	while (my $e = readdir $dir) {
		next unless $e =~ m/\.tgz$/;
		next unless -f "$dname/$e";
		push(@$l, $`);
	}
	close($dir);
	return $l;
}

package OpenBSD::PackageRepository::Local::Pipe;
our @ISA=qw(OpenBSD::PackageRepository::Local);

sub may_exist
{
	return 1;
}

sub open_pipe
{
	my ($self, $object) = @_;
	my $fullname = $self->{baseurl}.$object->{name};
	my $pid = open(my $fh, "-|");
	if (!defined $pid) {
		die "Cannot fork: $!";
	}
	if ($pid) {
		return $fh;
	} else {
		open STDERR, ">/dev/null";
		exec {"/usr/bin/gzip"} 
		    "gzip", 
		    "-d", 
		    "-c", 
		    "-q", 
		    "-f", 
		    "-"
		or die "can't run gzip";
	}
}

package OpenBSD::PackageRepository::Distant;
our @ISA=qw(OpenBSD::PackageRepository);

my $buffsize = 2 * 1024 * 1024;

sub pkg_copy
{
	my ($in, $dir, $name) = @_;

	require File::Temp;
	my $template = $name;
	$template =~ s/\.tgz$/.XXXXXXXX/;

	my ($copy, $filename) = File::Temp::tempfile($template,
	    DIR => $dir) or die "Can't write copy to cache";
	chmod 0644, $filename;
	my $handler = sub {
		my ($sig) = @_;
		unlink $filename;
		$SIG{$sig} = 'DEFAULT';
		kill $sig, $$;
	};

	my $nonempty = 0;
	{

	local $SIG{'PIPE'} =  $handler;
	local $SIG{'INT'} =  $handler;
	local $SIG{'HUP'} =  $handler;
	local $SIG{'QUIT'} =  $handler;
	local $SIG{'KILL'} =  $handler;
	local $SIG{'TERM'} =  $handler;

	my ($buffer, $n);
	# copy stuff over
	do {
		$n = sysread($in, $buffer, $buffsize);
		if (!defined $n) {
			die "Error reading\n";
		}
		if ($n > 0) {
			$nonempty = 1;
		}
		syswrite $copy, $buffer;
		syswrite STDOUT, $buffer;
	} while ($n != 0);
	close($copy);
	}

	if ($nonempty) {
		rename $filename, "$dir/$name";
	} else {
		unlink $filename;
	}
}

sub open_pipe
{
	require OpenBSD::Temp;

	my ($self, $object) = @_;
	$object->{errors} = OpenBSD::Temp::file();
	$object->{cache_dir} = $ENV{'PKG_CACHE'};
	my $pid = open(my $fh, "-|");
	if (!defined $pid) {
		die "Cannot fork: $!";
	}
	if ($pid) {
		$object->{pid} = $pid;
		return $fh;
	} else {
		open STDERR, '>', $object->{errors};

		my $pid2 = open(STDIN, "-|");

		if (!defined $pid2) {
			die "Cannot fork: $!";
		}
		if ($pid2) {
			exec {"/usr/bin/gzip"} 
			    "gzip", 
			    "-d", 
			    "-c", 
			    "-q", 
			    "-" 
			or die "can't run gzip";
		} else {
			if (defined $object->{cache_dir}) {
				my $pid3 = open(my $in, "-|");
				if (!defined $pid3) {
					die "Cannot fork: $!";
				}
				if ($pid3) {
					pkg_copy($in, $object->{cache_dir}, 
					    $object->{name});
					exit(0);
				} else {
					$self->grab_object($object);
				}
			} else {
				$self->grab_object($object);
			}
		}
	}
}

sub _list
{
	my ($self, $cmd) = @_;
	my $l =[];
	local $_;
	open(my $fh, '-|', "$cmd") or return undef;
	while(<$fh>) {
		chomp;
		next if m/^d.*\s+\S/;
		next unless m/([^\s]+)\.tgz\s*$/;
		push(@$l, $1);
	}
	close($fh);
	return $l;
}

sub finish_and_close
{
	my ($self, $object) = @_;
	if (defined $object->{cache_dir}) {
		while (defined $object->intNext()) {
		}
	}
	$self->SUPER::finish_and_close($object);
}

package OpenBSD::PackageRepository::HTTPorFTP;
our @ISA=qw(OpenBSD::PackageRepository::Distant);

our %distant = ();


sub grab_object
{
	my ($self, $object) = @_;
	my $ftp = defined $ENV{'FETCH_CMD'} ? $ENV{'FETCH_CMD'} : "/usr/bin/ftp";
	exec {$ftp} 
	    "ftp", 
	    "-o", 
	    "-", $self->{baseurl}.$object->{name}
	or die "can't run ftp";
}

sub maxcount
{
	return 1;
}

sub opened
{
	my $self = $_[0];
	my $k = $self->{key};
	if (!defined $distant{$k}) {
		$distant{$k} = [];
	}
	return $distant{$k};
}

sub _new
{
	my ($class, $baseurl) = @_;
	my $distant_host;
	if ($baseurl =~ m/^(http|ftp)\:\/\/(.*?)\//i) {
	    $distant_host = $&;
	}
	bless { baseurl => $baseurl, key => $distant_host }, $class;
}


package OpenBSD::PackageRepository::HTTP;
our @ISA=qw(OpenBSD::PackageRepository::HTTPorFTP);

sub list
{
	my ($self) = @_;
	if (!defined $self->{list}) {
		my $error = OpenBSD::Temp::file();
		$self->make_room();
		my $fullname = $self->{baseurl};
		my $l = $self->{list} = [];
		local $_;
		open(my $fh, '-|', "ftp -o - $fullname 2>$error") or return undef;
		# XXX assumes a pkg HREF won't cross a line. Is this the case ?
		while(<$fh>) {
			chomp;
			for my $pkg (m/\<A\s+HREF=\"(.*?)\.tgz\"\>/gi) {
				next if $pkg =~ m|/|;
				push(@$l, $pkg);
			}
		}
		close($fh);
		$self->parse_problems($error);
	}
	return $self->{list};
}

package OpenBSD::PackageRepository::FTP;
our @ISA=qw(OpenBSD::PackageRepository::HTTPorFTP);


sub list
{
	my ($self) = @_;
	if (!defined $self->{list}) {
		require OpenBSD::Temp;

		my $error = OpenBSD::Temp::file();
		$self->make_room();
		my $fullname = $self->{baseurl};
		$self->{list} = $self->_list("echo 'nlist *.tgz'|ftp -o - $fullname 2>$error");
		$self->parse_problems($error);
	}
	return $self->{list};
}

1;
