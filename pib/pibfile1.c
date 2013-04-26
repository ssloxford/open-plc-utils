/*====================================================================*
 *   
 *   Copyright (c) 2011 Qualcomm Atheros Inc.
 *   
 *   Permission to use, copy, modify, and/or distribute this software 
 *   for any purpose with or without fee is hereby granted, provided 
 *   that the above copyright notice and this permission notice appear 
 *   in all copies.
 *   
 *   THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL 
 *   WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED 
 *   WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL  
 *   THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR 
 *   CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM 
 *   LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, 
 *   NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN 
 *   CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *   
 *--------------------------------------------------------------------*/

/*====================================================================*
 *
 *   signed pibfile1 (struct _file_ const * file);
 *
 *   pib.h
 *
 *   open a thunderbolt/lightning PIB file and validate it by 
 *   checking file size, checksum and selected internal parameters; 
 *   return a file descriptor on success; terminate the program on 
 *   error;
 *
 *
 *   Contributor(s):
 *	Charles Maier <cmaier@qca.qualcomm.com>
 *
 *--------------------------------------------------------------------*/

#ifndef PIBFILE1_SOURCE
#define PIBFILE1_SOURCE

#include <unistd.h>
#include <errno.h>

#include "../tools/files.h"
#include "../tools/error.h"
#include "../pib/pib.h"

signed pibfile1 (struct _file_ const * file) 

{
	struct simple_pib simple_pib;
	if (lseek (file->file, 0, SEEK_SET)) 
	{
		error (1, errno, FILE_CANTHOME, file->name);
	}
	if (read (file->file, &simple_pib, sizeof (simple_pib)) != sizeof (simple_pib)) 
	{
		error (1, errno, FILE_CANTREAD, file->name);
	}
	if (lseek (file->file, 0, SEEK_SET)) 
	{
		error (1, errno, FILE_CANTHOME, file->name);
	}
	if ((simple_pib.RESERVED1) || (simple_pib.RESERVED2)) 
	{
		error (1, errno, PIB_BADCONTENT, file->name);
	}
	if (fdchecksum32 (file->file, LE16TOH (simple_pib.PIBLENGTH), 0)) 
	{
		error (1, errno, PIB_BADCHECKSUM, file->name);
	}
	if (lseek (file->file, 0, SEEK_SET)) 
	{
		error (1, errno, FILE_CANTHOME, file->name);
	}
	return (0);
}


#endif

