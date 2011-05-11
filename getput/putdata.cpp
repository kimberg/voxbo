
// putdata.cpp
// Copyright (c) 1998-2010 by The VoxBo Development Team

// This file is part of VoxBo
// 
// VoxBo is free software: you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// VoxBo is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with VoxBo.  If not, see <http://www.gnu.org/licenses/>.
// 
// For general information on VoxBo, including the latest complete
// source code and binary distributions, manual, and associated files,
// see the VoxBo home page at: http://www.voxbo.org/
// 
// original version written by Zack Smith

/* Note re errors
 *
 * Putdata is usually run from IDL with '&', meaning it runs concurrently
 * with the IDL program. Thus neither a numeric return code
 * nor stdout can be used to report an error back to the IDL program.
 * If putdata encounters a problem, it must return its error code
 * via the return pipe, assuming that the pipe exists.
 */



#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include <unistd.h>

#include <vbutil.h>
#include <vbio.h>

#include <vector>



//#define DEBUG



using namespace std; 

enum {
	TYPE_NONE=0,
	TYPE_3D=1,
	TYPE_4D=2,
	TYPE_2D=3,
	TYPE_1D=4,
};


static char *readfifopath = NULL;
static FILE *read_fifo = NULL;
static bool read_completed = false;
static char *return_path = NULL;

#define MSG(AAA) { fprintf(stderr, "Putdata fatal error: %s\n", AAA); fflush(stderr); }



#define WARNING(AAA) { fprintf(stderr, "Putdata warning: %s\n", AAA); fflush(stderr); }

enum {
	NOARG=1,
	BADPATH=2,
	ERROR_NOTUSED_ERROR_=3,
	NOSUPPORT=4,
	READERROR=5,
	BADOP=6,
	ERROR_WRITEOPEN=7,
	ERROR_READOPEN=8,
	ERROR_BADDIMS=9,
	ERROR_WRITEFAIL=10,
	ERROR_WRITEFAIL2=11,
	ERROR_BADTYPE=12,
	ERROR_NOMASK=13,
	ERROR_NOMEM=14,
	ERROR_NOPATHS=15,
	ERROR_BADHEADER=16,
	ERROR_EOF=17,
	ERROR_READFAIL=18
};

static char *errstr[19] = {
	"none",
	"missing arg",
	"bad path",
	"(not used)",
	"not supported",
	"read error",
	"bad operation #",
	"cannot open file for writing",
	"cannot open file for reading",
	"bad dimensions",
	"write failure 1",
	"write failure 2",
	"bad data type",
	"no mask",
	"out of memory",
	"no paths",
	"bad header",
	"end of file",
	"read failure"
};

enum {
	OP3D=1,	// 3d
	OP4D=2,	// 4d
	OP3D_TES=3
};


void complain (unsigned short code) 
{ 
	MSG(errstr[code]); 

	// If the data stream not yet been read,
	// let us do that in order to release the IDL
	// program from its blocked state.
	if (!read_completed && readfifopath) {
		if (!read_fifo) {
			read_fifo = fopen(readfifopath,"rb");
		}
		if (read_fifo) {
			int ch;
			while (EOF != (ch = fgetc(read_fifo)))
				;
			fclose(read_fifo);
			read_fifo = NULL;
			read_completed= true;
		}
	}

	// Send back the error code.
	if (return_path) {
		FILE *f = fopen(return_path,"w"); 
		if(f) {
			fprintf(f, "-%d\n", code); 
			fclose(f); 
		} else
			fprintf(stderr, "putdata: unable to send back error code (%d)\n",code);
	}

	exit(-1); 
}

int readline (FILE *f, char *buf, int lim)
{
	int ch, i=0;
	while (EOF != (ch = fgetc(f))) {
		if (i >= (lim-1))
			break;
		if (ch == 10)
			break;
		if (ch != 13)
			buf[i++] = ch;
	}
	buf[i]=0;
	if (!i && ch==EOF)
		return -1;
	return i;
}


int receive_3d (char *fifo, char *path, int dimx, int dimy, int dimz, int datatype)
{
	char fifo_from_idl [PATH_MAX];
	char fifo_to_idl [PATH_MAX];
	char line[200];
	int i;

	strcpy (fifo_from_idl, fifo );
	strcpy (fifo_to_idl, fifo );
	strcat (fifo_from_idl, "-");

	readfifopath = fifo_from_idl;
	return_path = fifo_to_idl;

	FILE *f = fopen (fifo_from_idl, "rb");
	if (!f) 
		complain(BADPATH);

	read_fifo = f;

#ifdef DEBUG
	fprintf (stderr, "Entered receive_3d\n");
	fflush (stderr);
#endif

	//----------------------------------------------
	// Check params
	//
	if (dimx <= 0 || dimy <= 0 || dimz <= 0)
		complain(ERROR_BADDIMS);
	if (datatype < 0)
		complain(ERROR_BADTYPE);

	//----------------------------------------------
	// Get data type & size.
	//
	VB_datatype dt2;
	int datasize=0;
	switch (datatype) {
	case 1:	dt2 = vb_byte; datasize=1; break;
	case 2:	dt2 = vb_short; datasize=2; break;
	case 3:	dt2 = vb_long; datasize=4; break;
	case 4:	dt2 = vb_float; datasize=sizeof(float); break;
	case 5:	dt2 = vb_double; datasize=sizeof(double); break;
	default:
                return ERROR_BADTYPE;
	}

	//----------------------------------------------
	// Get the user-header info.
	//
	vector<string> user_header;
	if (0 > readline(f, line, 199))
		complain(ERROR_EOF);
	int total_header_items = atoi(line);
	if (total_header_items < 0)
		complain(ERROR_BADHEADER);
	for (i=0; i < total_header_items; i++) {
		if (0 > readline(f, line, 199))
			complain(ERROR_EOF);
		user_header.push_back (string(line));
	}

	//----------------------------------------------
	// Create the Cube object & put the data in it.
	//
	Cube *c = new Cube(dimx,dimy,dimz,dt2);
	if (!c)
		complain(ERROR_NOMEM);

	//----------------------------------------------
	// Store the user header into the Cube object.
	//
	for (i=0; i < total_header_items; i++) {
		c->header.push_back (user_header[i]);
	}

	//----------------------------------------------
	// Now we process specific user header data
	// that we know have equivalents in the Tes object.
	//
	for (i=0; i < total_header_items; i++) {
		const char *s = user_header[i].c_str();
		char *s2 = s ? strchr(s, ':') : NULL;
		if (s2 && '*' == *s2)
			continue;
#ifdef DEBUG
		fprintf(stderr, "putdata got user header line: %s\n", s);
#endif
		if (!s2) s2 = s ? strchr (s, '=') : NULL;
		if (s2) {
			*s2++ = 0;
			if (!strcasecmp (s, "VoxSizes(xyz)")) {
				double x,y,z;
				if (3 == sscanf (s2, "%lg %lg %lg", &x,&y,&z)) {
					c->voxsize[0] = x;
					c->voxsize[1] = y;
					c->voxsize[2] = z;
				} else {
					WARNING("invalid voxsizes from user header.");
				}
			}
			else
			if (!strcasecmp (s, "Origin(xyz)")) {
				int x,y,z;
				if (3 == sscanf (s2, "%d %d %d", &x,&y,&z)) {
					c->origin[0] = x;
					c->origin[1] = y;
					c->origin[2] = z;
				} else {
					WARNING("invalid origin from user header.");
				}
			}
			else
			if (!strcasecmp (s, "Orientation")) {
				c->orient = s2;
			} 
			else
			if (!strcasecmp (s, "Scale")) {
				// float f = atof(s2);
				// c->scalefactor = f ? f : 1.0;
			} 
			else
			if (!strcasecmp (s, "ByteOrder")) {
				c->filebyteorder = atoi(s2)? ENDIAN_BIG : ENDIAN_LITTLE;
			}
		} 
	}

	//----------------------------------------------
	// Get the data
	//
	ulong chunk_size = dimx * dimy * dimz * datasize;
	unsigned char *chunk = (unsigned char*) malloc (chunk_size);
	if (!chunk) 
		complain(ERROR_NOMEM);
	if (chunk_size != fread (chunk,1,chunk_size,f))
		complain(ERROR_READFAIL);

	fclose (f);
	read_completed = true;

	c->data = chunk;

	//----------------------------------------------
	// Write the Cube to a file.
	//
	c->SetFileName (path);
#ifdef DEBUG
	fprintf(stderr, "Calling write cube to file %s\n", path);
#endif
	c->WriteFile ();

	//----------------------------------------------
	// Return the success code
	//
	f = fopen (fifo_to_idl, "wb");
	if (!f)
		return 0;
	fprintf (f, "0\n");
	fflush(f);
	fclose(f);

	delete c;

	return 0;
}


int receive_4d (char *fifo, char *path, int dimx, int dimy, int dimz, int dimt, int datatype)
{
	char fifo_from_idl [PATH_MAX];
	char fifo_to_idl [PATH_MAX];
	char line[200];
	int i;

	strcpy (fifo_from_idl, fifo );
	strcpy (fifo_to_idl, fifo );
	strcat (fifo_from_idl, "-");

#ifdef DEBUG
	fprintf (stderr, "Entered receive_4d, fifo_from_idl=%s\n", fifo_from_idl);
	fflush (stderr);
#endif

	FILE *f = fopen (fifo_from_idl, "rb");
	if (!f) 
		complain(BADPATH);

	//----------------------------------------------
	// Check params
	//
	if (dimx <= 0 || dimy <= 0 || dimz <= 0 || dimt <= 0)
		complain(ERROR_BADDIMS);
	if (datatype < 0)
		complain(ERROR_BADTYPE);

	//----------------------------------------------
	// Convert type
	//
	VB_datatype dt2;
	int datasize=0;
	switch (datatype) {
	case 1:	dt2 = vb_byte; datasize=1; break;
	case 2:	dt2 = vb_short; datasize=2; break;
	case 3:	dt2 = vb_long; datasize=4; break;
	case 4:	dt2 = vb_float; datasize=sizeof(float); break;
	case 5:	dt2 = vb_double; datasize=sizeof(double); break;
	default:
                return ERROR_BADTYPE;
	}

	//----------------------------------------------
	// Get the user-header info.
	//
	vector<string> user_header;
	if (0 > readline(f, line, 199))
		complain(ERROR_EOF);
	int total_header_items = atoi(line);
	if (total_header_items < 0)
		complain(ERROR_BADHEADER);
	for (i=0; i < total_header_items; i++) {
		if (0 > readline(f, line, 199))
			complain(ERROR_EOF);
		user_header.push_back (string(line));
	}

	//----------------------------------------------
	// Get the data
	//
	ulong chunk_size = dimx * dimy * dimz * dimt * datasize;
	unsigned char *chunk = (unsigned char*) malloc (chunk_size);
	if (!chunk) 
		complain(ERROR_NOMEM);
	if (chunk_size != fread (chunk,1,chunk_size,f))
		complain(ERROR_READFAIL);
	fclose (f);

	//----------------------------------------------
	// Create the Tes
	//
	Tes *t = new Tes(dimx,dimy,dimz,dimt,dt2);
	if (!t)
		complain(ERROR_NOMEM);

	//----------------------------------------------
	// Store the user header into the Tes object.
	//
	for (i=0; i < total_header_items; i++) {
		t->header.push_back (user_header[i]);
	}

	//----------------------------------------------
	// Now we process specific user header data
	// that we know have equivalents in the Tes object.
	//
	for (i=0; i < total_header_items; i++) {
		const char *s = user_header[i].c_str();
		char *s2 = s ? strchr(s, ':') : NULL;
		if (s2 && '*' == *s2)
			continue;
		if (!s2) s2 = s ? strchr (s, '=') : NULL;
		if (s2) {
			*s2++ = 0;
			if (!strcasecmp (s, "VoxSizes(xyz)")) {
				double x,y,z;
				if (3 == sscanf (s2, "%lg %lg %lg", &x,&y,&z)) {
					t->voxsize[0] = x;
					t->voxsize[1] = y;
					t->voxsize[2] = z;
				} else {
					WARNING("invalid voxsizes from user header.");
				}
			}
			else
			if (!strcasecmp (s, "Origin(xyz)")) {
				int x,y,z;
				if (3 == sscanf (s2, "%d %d %d", &x,&y,&z)) {
					t->origin[0] = x;
					t->origin[1] = y;
					t->origin[2] = z;
				} else {
					WARNING("invalid origin from user header.");
				}
			}
			else
			if (!strcasecmp (s, "Orientation")) {
				t->orient = s2;
			} 
			else
			if (!strcasecmp (s, "Scale")) {
				// float f = atof(s2);
				// t->scalefactor = f ? f : 1.0;
			} 
			else
			if (!strcasecmp (s, "ByteOrder")) {
				t->filebyteorder = atoi(s2)? ENDIAN_BIG : ENDIAN_LITTLE;
			} 
		} 
	}

#ifdef DEBUG
	time_t t0 = time(NULL);
#endif

	//----------------------------------------------
	// Store the data into the Tes.
	//
	int x,y,z,t2;
	//unsigned dimxy = dimx*dimy;
	//unsigned dimxyz = dimxy * dimz;
	for (x=0; x<dimx; x++) {
		for (y=0; y<dimy; y++) {
			for (z=0; z<dimz; z++) {
				for (t2=0; t2<dimt; t2++) {
					double value=0.0;
					unsigned long ix = t2 + x*dimt + y*dimt*dimx + z*dimt*dimx*dimy;

					switch (datatype) {
					case 1: 
						value = chunk[ix]; 
						break;
					case 2: 
						value = ((short*)chunk)[ix]; 
						break;
					case 3: 
						value = ((long*)chunk)[ix]; 
						break;
					case 4: 
						value = ((float*)chunk)[ix]; 
						break;
					case 5: 
						value = ((double*)chunk)[ix]; 
						break;
					}
					t->SetValue (x,y,z,t2, value);
				}
			}
		}
	}

#ifdef DEBUG
	t0 -= time(NULL);
	fprintf(stderr, "Time to write data into Tes object: %ld seconds\n", (long)-t0);
	fflush(stderr);
#endif

	//----------------------------------------------
	// Write the Tes to the file.
	//
	t->SetFileName (path);
#ifdef DEBUG
	fprintf(stderr, "Writing Tes file to %s\n", path);
#endif
	t->WriteFile ();

	//----------------------------------------------
	// Return the success code
	//
	f = fopen (fifo_to_idl, "wb");
	if (!f)
		return 0;
	fprintf (f, "1\n");
	fflush(f);
	fclose(f);

	delete t;

	return 0;
}



int
main(int argc, char **argv)
{
	char *fifopath;

#ifdef DEBUG
	fprintf (stderr, "Putdata started.\n");
	fflush (stderr);
#endif

	if (argc != 10)
		complain(NOARG);

	return_path = fifopath = argv[2];

	char *path = argv[1];
	char len = strlen(path);
	if (len < 4)
		complain(BADPATH);

	// int op = atoi(argv[3]);

	int dimx = atoi(argv[4]);
	int dimy = atoi(argv[5]);
	int dimz = atoi(argv[6]);
	int dimt = atoi(argv[7]);
// fprintf (stderr, "putdata: dimx=%d dimy=%d dimz=%d dimt=%d\n", dimx,dimy,dimz,dimt);
	int datatype = atoi(argv[8]);
	int dimensions = atoi(argv[9]);

	int child;
	if (!(child = fork())) {
#ifdef DEBUG
		fprintf (stderr, "In child process, dimensions=%d\n", dimensions);
		fflush (stderr);
#endif

		switch (dimensions) {
		case 3:
			dimt = 1;
			receive_3d (fifopath, path, dimx,dimy,dimz,datatype);
			break;
		
		case 4:
			receive_4d (fifopath, path, dimx,dimy,dimz,dimt, datatype);
			break;

		default:
			complain(NOSUPPORT);
			break;
		}

#ifdef DEBUG
		fprintf (stderr, "Leaving child process.\n");
		fflush (stderr);
#endif
	}

#ifdef DEBUG
	fprintf (stderr, "Putdata exiting.\n");
	fflush (stderr);
#endif

	return 0;
}

