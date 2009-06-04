/*
Copyright (C) 2003-2004 Douglas Thain and the University of Wisconsin
Copyright (C) 2005- The University of Notre Dame
This software is distributed under a BSD-style license.
See the file COPYING for details.
*/

#ifndef DEBUG_H
#define DEBUG_H

/** @file debug.h General purpose debugging routines.
The cctools debugging system is built into all software components.
Any code may invoke @ref debug with a printf-style message to log relevant
information.  Each debug call uses a flag to indicate which subsystem is
doing the logging, so that various subsystems may be easily turned on and off.
For example, the Chirp subsystem has many statements like this:

<pre>
debug(D_CHIRP,"reading file %s from host %s:d",filename,hostname,port);
</pre>

The <tt>main</tt> routine of a program is responsible for
calling @ref debug_config, @ref debug_config_file and @ref debug_flags_set to choose
what to display and where to send it.  By default, nothing is displayed,
unless it has the flags D_NOTICE or D_FATAL.  For example, a main program might do this:

<pre>
  debug_config("superprogram");
  debug_config_file("/tmp/myoutputfile");
  debug_flags_set("tcp");
  debug_flags_set("chirp");
</pre>
*/

#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>

#define D_SYSCALL  0x00000001  /**< Debug system calls in Parrot. */
#define D_CHANNEL  0x00000002  /**< Debug the I/O channel in Parrot. */
#define D_PROCESS  0x00000004  /**< Debug jobs and process. */
#define D_NOTICE   0x00000008  /**< Indicates a message that is always shown. */
#define D_RESOLVE  0x00000010  /**< Debug the file name resolver in Parrot. */
#define D_LIBCALL  0x00000020  /**< Debug I/O library calls in Parrot. */
#define D_LOCAL    0x00000040  /**< Debug the local I/O module in Parrot. */
#define D_DNS      0x00000080  /**< Debug domain name lookups. */
#define D_TCP      0x00000100  /**< Debug TCP connections and disconnections. */
#define D_AUTH     0x00000200  /**< Debug authentication and authorization actions. */
#define D_IRODS    0x00000400  /**< Debug the iRODS module in Parrot. */
#define D_LANDLORD 0x00000800  /**< Debug Landlord operations. */
#define D_HTTP     0x00001000  /**< Debug HTTP queries. */
#define D_FTP      0x00002000  /**< Debug FTP operations. */
#define D_NEST     0x00004000  /**< Debug the NEST module in Parrot. */
#define D_GROW     0x00008000  /**< Debug the GROW filesystem in Parrot. */
#define D_CHIRP    0x00010000  /**< Debug Chirp protocol operations. */
#define D_DCAP     0x00020000  /**< Debug the DCAP module in Parrot. */
#define D_RFIO     0x00040000  /**< Debug the RFIO module in Parrot. */
#define D_GLITE    0x00080000  /**< Debug the gLite module in Parrot. */
#define D_MULTI    0x00100000  /**< Debug Chirp Multi filesystems. */
#define D_PSTREE   0x00200000  /**< Debug process trees in Parrot. */
#define D_ALLOC    0x00400000  /**< Debug space allocations in the Chirp server. */
#define D_LFC	   0x00800000  /**< Debug LFC file lookups in Parrot. */
#define D_GFAL	   0x01000000  /**< Debug the GFAL module in Parrot. */
#define D_SUMMARY  0x02000000  /**< Show I/O summary stats in Parrot. */
#define D_DEBUG    0x04000000  /**< Show general debugging messages. */
#define D_LOGIN    0x08000000  /**< Debug logins on the Chirp server. */
#define D_CACHE    0x10000000  /**< Debug cache operations in Parrot. */
#define D_POLL     0x20000000  /**< Debug FD polling in Parrot. */
#define D_HDFS	   0x40000000  /**< Debug the HDFS module in Parrot. */

/** Debug all remote I/O operations. */
#define D_REMOTE   (D_HTTP|D_FTP|D_NEST|D_CHIRP|D_DCAP|D_RFIO|D_LFC|D_GFAL|D_MULTI|D_GROW|D_IRODS|D_HDFS)

/** Show all debugging info. */
#define D_ALL      0xffffffff

/*
It turns out that many libraries and tools make use of
symbols like "debug" and "fatal".  This causes strange
failures when we link against such codes.  Rather than change
all of our code, we simply insert these defines to
transparently modify the linker namespace we are using.
*/

#define debug                  cctools_debug
#define fatal                  cctools_fatal
#define debug_config           cctools_debug_config
#define debug_config_file      cctools_debug_config_file
#define debug_config_file_size cctools_debug_config_file_size
#define debug_config_fatal     cctools_debug_config_fatal
#define debug_config_getpid    cctools_debug_config_getpid
#define debug_flags_set        cctools_debug_flags_set
#define debug_flags_print      cctools_debug_flags_print
#define debug_flags_clear      cctools_debug_flags_clear
#define debug_flags_restore    cctools_debug_flags_restore

/** Emit a debugging message.
Logs a debugging message, if the given flags are active.
@param flags Any of the standard debugging flags OR-ed together.
@param fmt A printf-style formatting string, followed by the necessary arguments.
*/

void debug( int flags, const char *fmt, ... );

/** Emit a fatal debugging message and exit.
Displays a printf-style message, and then forcibly exits the program.
@param fmt A printf-style formatting string, followed by the necessary arguments.
*/

void fatal( const char *fmt, ... );

/** Initialize the debugging system.
Must be called before any other calls take place.
@param name The name of the program to use in debug output.
*/

void debug_config( const char *name );

/** Direct debug output to a file.
All enabled debugging statements will be sent to this file.
@param file The pathname of the file for output.
@see debug_config_file_size
*/

void debug_config_file( const char *file );

/** Set the maximum debug file size.
Debugging files can very quickly become large and fill up your available disk space.
This functions sets the maximum size of a debug file.
When it exceeds this size, it will be renamed to (file).old, and a new file will be started.
@param size Maximum size in bytes of the debugging file.
*/

void debug_config_file_size( int size );

void debug_config_fatal( void (*callback) () );
void debug_config_getpid( pid_t (*getpidfunc)() );

/** Set debugging flags to enable output.
Accepts a debug flag in ASCII form, and enables that subsystem.  For example: <tt>debug_flags_set("chirp");</tt>
Typically used in command-line processing in <tt>main</tt>.
@param flagname The name of the debugging flag to enable.
@return One if the flag is valid, zero otherwise.
@see debug_flags_print, debug_flags_clear
*/

int  debug_flags_set( const char *flagname );

/** Display the available debug flags.
Prints on the standard output all possible debug flag names that
can be passed to @ref debug_flags_set.  Useful for constructing a program help text.
@param stream Standard I/O stream on which to print the output.
*/

void debug_flags_print( FILE *stream );

/** Clear all debugging flags.
Clear all currently set flags, so that no output will occur.
@see debug_flags_set
*/

int  debug_flags_clear();

#endif

