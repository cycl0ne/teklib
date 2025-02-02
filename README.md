# teklib
Clone of teklib project at http://hg.neoscientists.org/teklib 
from 2012 for Archive V3

--

= About TEKlib =

TEKlib is an open-source library under the terms of the free MIT
software license. By picking from its facilities, it can serve as a

	* Middleware

	* Embedded operating system

	* Virtual OS target for applications and libraries that would
	have to be ported to many individual hosts

	* Component architecture and hosting environment to allow plug-ins
	and their hosts to be written in a platform-independent manner

Its freestanding nature, small resource footprint and absence of
global data recommend it as a layer for portability and code reuse in
a wide range of environments; it can be used in userspace
applications, kernel modules, plug-ins, device drivers and on raw
hardware.

== Features ==

	* Recursive memory management: Allocators can be stacked on top of
	each other and carry attributes for automatic cleanup and
	thread-safety; special allocators can implement tighter packing,
	custom allocation strategies, leak tracking, or operate in
	user-supplied memory

	* TEKlib threads (''tasks'') are 'heavy-weight', or process-like.
	They are equipped with means for local storage, individual file
	descriptors, a reserved set of signals and endpoints for
	communication. User signals can be synchronized on individually, in
	sets or by using messages and I/O packets

	* TEKlib provides strict asynchronoucy and atomicity throughout its
	API; the core as a whole never blocks due to latency inflicted by
	I/O, and it never imposes polling as an inevitable mechanism

	* The module interface is inherently thread-safe and provides a
	referencing scheme for any number of tasks requesting a component,
	library or device driver at the same time

	* Recursive mutexes and an extension thereof, ''atoms'', allow for
	lookup by name, locking attempts and shared resource arbitration

	* An asynchronous device driver interface can be utilized by
	applications for overlapping I/O; using this interface, TEKlib also
	implements a portable timer device

	* File systems are an extension to device drivers. TEKlib provides
	a file system namespace for any number of mountpoints (''devices'')
	and logical roots (''assigns''). ''Handlers'' are modules
	implementing roots in the file system; they can be mounted
	explicitely or initialize themselves on demand. In hosted
	scenarios, the file system is initially populated with a default
	handler to occupy the root node, thus abstracting from a host's
	native file system.

	* Inbuilt data structures such as doubly-linked lists, arrays of
	key/value pairs, nodes with userdata and destructor, hashes
	and callback hooks

	* TEKlib comes with its own, entirely self-contained makefile
	generator.

== Availability ==

	* TEKlib is freely available for free software as well as academic
	and commercial purposes. See COPYRIGHT for details.

	* TEKlib can be downloaded from http://www.teklib.org/ .

== Installation ==

	* Supported make tools are {{pmake}} and {{gmake}}.

	* Type {{make}} at the top-level directory to see a list of
	possible configuration variables and build targets.

	* Adjust the configuration in the Makefile, or pass variables to
	your make tool; for example,

	using pmake:

			# make HOST=fbsd release

	Using bash and GNU make:

			# HOST=linux make release

== Authors and contact ==

	* TEKlib was originally designed by Timm S. Mueller, and over the
	years received support from a number of contributors, in
	particular, but not limited to: Daniel Adler, Daniel Trompetter,
	Franciska Schulze, Frank Pagels, Tobias Schwinger, Michael Taubert

	* Send comments, questions and bug reports to teklib at
	teklib.org .

