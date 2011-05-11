
// getdata.cpp
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
// written by Zack Smith

/* Note re file errors
 *
 * Getdata is usually run from IDL with '&', meaning it runs concurrently
 * with the IDL program. Thus neither a numeric return code
 * nor stdout can be used to report an error back to the IDL program.
 * If getdata encounters a problem, it must return its error code
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

#include "time_series_avg.h"

using namespace std; 


enum {
	TYPE_NONE=0,
	TYPE_TSRA=1,
	TYPE_1D=2,
	TYPE_2D=3,
	TYPE_3D=4,
	TYPE_4D=5
};


static char *fifopath = NULL;
static bool return_fifo_opened = false;
static FILE *return_fifo = NULL;

#define MSG(AAA) { fprintf(stderr, "Getdata fatal error: %s\n", AAA); fflush(stderr); }

// This macro is for when we are able to provide
// to the caller a numeric error code.
#define COMPLAIN(AAA) { MSG(errstr[AAA]); if(return_fifo){fprintf(return_fifo,"-%d\n",AAA);fclose(return_fifo);} else { if (!return_fifo_opened) { FILE *f = fopen(fifopath,"w"); if(f) fprintf(f, "-%d\n", AAA); } } exit(-1); }


//#define DEBUG


#ifdef DEBUG
  #define DEBUG_MESSAGE(AAA) {fprintf(stderr,"Getdata debug: %s\n",AAA);fflush(stderr); }
#else
  #define DEBUG_MESSAGE(AAA) { }
#endif


enum {
	NOARG=1,
	BADPATH=2,
	ERR_unused=3,
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
};

static char *errstr[16] = {
	"none",
	"missing argument",
	"bad path",
	"bad extension",
	"no support",
	"read error",
	"bad operation",
	"cannot open file for writing",
	"cannot open file for reading",
	"bad dimensions",
	"write failure 1",
	"write failure 2",
	"bad datatype",
	"no mask",
	"no memory",
	"missing paths"
};

enum {
	OP_DATA=2,	// send data
	OP_MASK=4,	// send mask
	OP_TSRA=8,	// send time series & regional average
	OP_TS=1,	// time series flag
	OP_MN=2,	// mean norm flag
};



// Time series reader state machine's states.
enum {
	STATE_READING_PATHS=0,
	STATE_READING_COORDS=1,
};



void deconstruct_voxel_position (int pos, int& x, int& y, int& z,
		int dimx, int dimy, int dimz)
{
	x = pos % dimx;
	pos /= dimx;
	y = pos % dimy;
	pos /= dimy;
	z = pos % dimz;
}


void send_userhdr (FILE *f, string s)
{
	fprintf (f, "%%%s\n", s.c_str());
}


int send_timeseries (char *fifopath, int op)
{
	int i;
	vector <double> average;
	vector <vector <double > > time_series;
	vector <string> paths;
	vector <unsigned long> coords; // AKA regions
	int total_paths = 0;
	int total_coords = 0;
	char fifo_to_idl [PATH_MAX];
	char fifo_from_idl [PATH_MAX];
	bool doing_ts = (op & OP_TS) ? true : false;
	bool doing_mn = (op & OP_MN) ? true : false;

	DEBUG_MESSAGE("child entered send-time-series")

	strcpy (fifo_to_idl, fifopath);
	strcpy (fifo_from_idl, fifopath);
	strcat (fifo_from_idl, "-");

	bool got_total_paths = false;
	bool got_total_coords = false;
	int state = 0;

#ifdef DEBUG
		fprintf(stderr, "getdata: reading from %s\n", fifo_from_idl);
		fflush(stderr);
#endif
	FILE *fr = fopen (fifo_from_idl, "rb");
	if (!fr) {
		perror ("fopen");
		return ERROR_WRITEOPEN;
	}

	DEBUG_MESSAGE("child opened fifo for reading")

	int j=0;
	while (true) {
		char buf[1000];
		int ch, i=0;
		while (EOF != (ch = fgetc(fr))) {
			if (ch == 10)
				break;
			if (ch != 13)
				buf[i++] = ch;
		}
		buf[i]=0;

#ifdef DEBUG
		fprintf(stderr, "getdata: got line '%s'\n", buf);
		fflush(stderr);
#endif

		if (ch == EOF)
			break;
		if (!i)
			break;

		if (!got_total_paths) {
			got_total_paths = true;
			j = total_paths = atoi (buf);
			continue;
		}
		if (!got_total_coords) {
			got_total_coords = true;
			total_coords = atoi (buf);
			continue;
		}

		if (state == STATE_READING_PATHS) {
#ifdef DEBUG
			fprintf(stderr, "Adding path %s\n", buf);
#endif
			paths.push_back (string(buf));
			j--;
			if (j <= 0) {
				state = STATE_READING_COORDS;
				j = total_coords;
			}
		} else {
			coords.push_back (atoi(buf));
			j--;
			if (j <= 0) 
				break;
		}
	}

	DEBUG_MESSAGE("child done reading fifo, opening for write")
	fclose (fr);

	FILE *f = fopen (fifo_to_idl, "wb");
	if (!f) {
		perror ("fopen");
		return ERROR_WRITEOPEN;
	}

	if (total_paths <= 0 || total_coords <= 0) {
		fprintf (f, "error=999\n");
		fclose (f);
		return ERROR_NOPATHS;
	}

	return_fifo_opened = true;
	return_fifo = f;

	// Get data type.
	char datatype = -1;
	char datasize = 0;
	long dimt;
	Tes *t = new Tes();
	t->ReadHeader (paths[0]);
	dimt = t->dimt;
	datatype = t->datatype;
	switch (datatype) {
	case vb_byte:	datatype = 0; datasize = 1; break;
	case vb_short:	datatype = 1; datasize = 2; break;
	case vb_long:	datatype = 2; datasize = 4; break;
	case vb_float:	datatype = 3; datasize = 4; break;
	case vb_double:	datatype = 4; datasize = 8; break;
	}
	if (!datasize) {
		return_fifo = NULL;
		fclose(f);
		return ERROR_BADTYPE;
	}
	if (dimt <= 0)
		dimt = 1;

	fprintf (f, "datatype=%d\n", datatype);
	fprintf (f, "coordinates=%d\n", total_coords);

#ifdef DEBUG
	fprintf(stderr, "getdata: #frames=%d, datatype=%d\n", dimt,datatype);
#endif

	if (doing_ts) {
		if (doing_mn) {
			i = regionalTimeSeries(coords, paths, time_series, true);
		} else {
			i = regionalTimeSeries(coords, paths, time_series, false);
		}
	} else {
		if (doing_mn) {
			i = regionalAverage(coords, paths, & average, true);
		} else {
			i = regionalAverage(coords, paths, & average, false);
		}
	}

	if (doing_ts) {
		int lim = time_series.size();
		int lim2 = (time_series[0]).size();
		fprintf (f, "count2=%d\n", lim);
		fprintf (f, "count=%d\n", lim2);
		for (i=0; i< lim; i++) {
			for (j=0; j< lim2; j++) {
				double value = time_series[i][j];
				fwrite ((void*)&value, 1, sizeof(double), f);
			}
		}
	} else {
		int lim = average.size();
		fprintf (f, "count=%d\n", lim);
		for (i=0; i< lim; i++) {
			double value = average[i];
			fwrite ((void*)&value, 1, sizeof(double), f);
		}
	}

	return_fifo = NULL;
	fflush (f);
	fclose (f);
	delete t;

	DEBUG_MESSAGE("child done writing fifo")
	return 0;
}


int send_4d (char *path_, char *fifopath, short op)
{
	DEBUG_MESSAGE("Child entered send-4d routine.")

	Tes *t = new Tes();
	const string path(path_);
	int err;
	bool do_data = (op & OP_DATA) ? 1 : 0;
	bool do_timeseries = (op & OP_TSRA) ? 1 : 0;
	int x, y, z, t2;

#ifdef DEBUG
	fprintf(stderr, "op=%d\n", op);
	fflush(stderr);
#endif

	if (do_timeseries)
		return BADOP;

	DEBUG_MESSAGE("child will open fifo for writing")

	FILE *f = fopen (fifopath, "wb");
	if (!f) {
		perror ("fopen");
		delete t;
		return ERROR_WRITEOPEN;
	}

	return_fifo_opened = true;
	return_fifo = f;

	DEBUG_MESSAGE("child open successful")

	// If all we need is the header, then read only that.
	if (do_data)
		err = t->ReadFile (path);
	else
		err = t->ReadHeader (path);

	if (err) {
		return_fifo=NULL;
		fclose (f);
		delete t;
		return ERROR_READOPEN;
	}

	int dimx, dimy, dimz, dimt;
	ulong count;
	dimx = t->dimx;
	dimy = t->dimy;
	dimz = t->dimz;
	dimt = t->dimt;

	// If it's a 3D Tes file, dimt should be == 1
	if (dimt < 1)
		dimt = 1;

	// Four dimensions
	if (dimx <= 0 || dimx <= 0 || dimz <= 0) {
#ifdef DEBUG
		fprintf(stderr, "Bad dims: x %d y %d z %d t %d\n", dimx,dimy,dimz,dimt);
		fflush (stderr);
#endif
		return_fifo = NULL;
		fclose(f);
		delete t;
		return ERROR_BADDIMS;
	}

	char datatype = -1;
	char datasize = 0;
	switch (t->datatype) {
	case vb_byte:	datatype = 0; datasize = 1; break;
	case vb_short:	datatype = 1; datasize = 2; break;
	case vb_long:	datatype = 2; datasize = 4; break;
	case vb_float:	datatype = 3; datasize = 4; break;
	case vb_double:	datatype = 4; datasize = 8; break;
	}
	if (!datasize) {
		return_fifo = NULL;
		fclose(f);
		delete t;
		return ERROR_BADTYPE;
	}

	if (t->voxsize) {
		fprintf (f, "voxsize_x=%g\n", t->voxsize[0]);
		fprintf (f, "voxsize_y=%g\n", t->voxsize[1]);
		fprintf (f, "voxsize_z=%g\n", t->voxsize[2]);
	}
	if (t->origin) {
		fprintf (f, "origin_x=%d\n", t->origin[0]);
		fprintf (f, "origin_y=%d\n", t->origin[1]);
		fprintf (f, "origin_z=%d\n", t->origin[2]);
	}

	fprintf (f, "filetype=TES1\n");
	fprintf (f, "major=%d\n", t->fileformat.version_major);
	fprintf (f, "minor=%d\n", t->fileformat.version_minor);
	fprintf (f, "dimensions=%d\n", t->fileformat.dimensions);
	fprintf (f, "dimx=%u\n", dimx);
	fprintf (f, "dimy=%u\n", dimy);
	fprintf (f, "dimz=%u\n", dimz);
	fprintf (f, "dimt=%u\n", dimt);
	fprintf (f, "datatype=%d\n", datatype);
	fprintf (f, "orientation=%s\n", t->orient.c_str());
	fprintf (f, "filename=%s\n", t->filename.c_str());
	// fprintf (f, "scalefactor=%f\n", t->scalefactor);
	fprintf (f, "endianness=%d\n", t->filebyteorder == ENDIAN_LITTLE ? 0 : 1);

	int total = t->header.size();
	int i;
	for (i = 0; i < total; i++)
		send_userhdr (f, t->header[i]);

	// Count the voxels
	// If we have a mask, this takes a little effort.
	int n_voxels = 0;
	if (!t->mask)
		n_voxels = dimx*dimy*dimz;
	else {
		for (z=0; z < dimz; z++) {
			for (y=0; y < dimy; y++) {
				for (x=0; x < dimx; x++) {
					if (t->GetMaskValue (x,y,z))
						n_voxels++;
				}
			}
		}
	}

	count = n_voxels * datasize;
	fprintf (f, "bytes=%lu\n", count);
	fprintf (f, "nvoxels=%lu\n", (ulong) n_voxels);
	fprintf (f, "data_start=%d\n", do_data ? 1 : 0);

	ulong n = dimx*dimy*dimz;
	if (!t->mask) {
		fprintf(f, "NO MASK\n");
		return_fifo = NULL;
		fclose(f);
		delete t;
		return ERROR_NOMASK;
	} else {
		ulong ix=0;
		ulong amt_to_try=n;
		while (ix < n) {
			long got = fwrite (t->mask+ix, 1, amt_to_try, f);
			if (got <= 0) {
				return_fifo = NULL;
				fclose(f);
				delete t;
				return ERROR_WRITEFAIL2;
			}
			ix += got;
			amt_to_try = n - ix;
		}
	}

	if (do_data) {
		ulong amt = n * dimt * datasize;
		void *data = malloc (amt);

		amt = dimx * dimy * dimz * dimt * datasize;
		if (amt < 1) {
#ifdef DEBUG
			fprintf(stderr, "Bad dims: x %d y %d z %d t %d\n", dimx,dimy,dimz,dimt);
			fflush (stderr);
#endif
			return_fifo = NULL;
			fclose(f);
			delete t;
			return ERROR_BADDIMS;
		}
		data = malloc (amt);
		if (!data) {
			return_fifo = NULL;
			fclose(f);
			delete t;
			return ERROR_NOMEM;
		}

		memset (data,0,amt);

		for (z=0; z < dimz; z++) {
			for (y=0; y < dimy; y++) {
				for (x=0; x < dimx; x++) {
					if (t->GetMaskValue (x,y,z)) {

						t->GetTimeSeries (x,y,z);

						for (t2=0; t2 < dimt; t2++) {
							double value = t->timeseries(t2);
							char *dc;
							short *ds;
							long *dl;
							float *df;
							double *dd;

							long ix = t2 + x*dimt + y*dimt*dimx + z*dimt*dimx*dimy;

							switch (t->datatype) {
							case vb_byte:	
								dc = ((char*)data) + ix;
								*dc = (char) value;
								break;
							case vb_short:	
								ds = ((short*)data) + ix;
								*ds = (short) value;
								break;
							case vb_long:	
								dl = ((long*)data) + ix;
								*dl = (long) value;
								break;
							case vb_float:	
								df = ((float*)data) + ix;
								*df = (float) value;
								break;
							case vb_double:	
								dd = ((double*)data) + ix;
								*dd = value;
								break;
							}
						}
					}
				}
			}
		}

		if (amt != fwrite (data,1,amt,f)) {
			return_fifo = NULL;
			fclose(f);
			free (data);
			delete t;
			return ERROR_WRITEFAIL;
		}

		free (data);
	}

	return_fifo = NULL;
	fflush (f);
	fclose (f);
	DEBUG_MESSAGE("child write of 4d file header successful.")

	delete t;
	return 0;
}


bool send_3d (char *path_, char *fifopath, short op)
{
	Cube *c = new Cube();
	const string path(path_);
	int err=0;
	bool do_data = op & OP_DATA;

	DEBUG_MESSAGE("Child entered send-3d routine.")

	if (do_data) {
		err = c->ReadFile (path);
		if (err) 
			DEBUG_MESSAGE("Child's attempt to read file failed.")
	} else {
		err = c->ReadHeader (path);
		if (err) 
			DEBUG_MESSAGE("Child's attempt to read header failed.")
	}

	DEBUG_MESSAGE("Child will write 3d file info.")

	FILE *f = fopen (fifopath, "wb");
	if (!f) {
		DEBUG_MESSAGE("Child unable to open file.")
		perror ("fopen");
		return false;
	}

	return_fifo = f;

	DEBUG_MESSAGE("Child opened fifo for writing.")

	int dimx, dimy, dimz, dimt;
	ulong count;
	dimx = c->dimx;
	dimy = c->dimy;
	dimz = c->dimz;
	dimt = c->dimt;

	// Three dimensions
	if (dimx <= 0 || dimx <= 0 || dimz <= 0) {
		delete c;
		return_fifo = NULL;
		fclose (f);
#ifdef DEBUG
		fprintf(stderr, "Bad dims: x %d y %d z %d\n", dimx,dimy,dimz);
		fflush (stderr);
#endif
		return false;
	}

	char datatype = -1;
	char datasize = 0;
	switch (c->datatype) {
	case vb_byte:	datatype = 0; datasize = 1; break;
	case vb_short:	datatype = 1; datasize = 2; break;
	case vb_long:	datatype = 2; datasize = 4; break;
	case vb_float:	datatype = 3; datasize = 4; break;
	case vb_double:	datatype = 4; datasize = 8; break;
	}
	if (!datasize) {
		delete c;
		return_fifo = NULL;
		fclose (f);
#ifdef DEBUG
		fprintf(stderr, "Bad datatype: %d\n", c->datatype);
		fflush (stderr);
#endif
		return false;
	}
	count = dimx * dimy * dimz * datasize;

	if (c->voxsize) {
		fprintf (f, "voxsize_x=%g\n", c->voxsize[0]);
		fprintf (f, "voxsize_y=%g\n", c->voxsize[1]);
		fprintf (f, "voxsize_z=%g\n", c->voxsize[2]);
	}
	if (c->origin) {
		fprintf (f, "origin_x=%d\n", c->origin[0]);
		fprintf (f, "origin_y=%d\n", c->origin[1]);
		fprintf (f, "origin_z=%d\n", c->origin[2]);
	}

	fprintf (f, "filetype=CUB1\n");
	fprintf (f, "major=%d\n", c->fileformat.version_major);
	fprintf (f, "minor=%d\n", c->fileformat.version_minor);
	fprintf (f, "dimx=%u\n", dimx);
	fprintf (f, "dimy=%u\n", dimy);
	fprintf (f, "dimz=%u\n", dimz);
	fprintf (f, "dimt=%u\n", dimt);
	fprintf (f, "dimensions=%u\n", c->fileformat.dimensions);
	fprintf (f, "bytes=%lu\n", count);
	fprintf (f, "datatype=%d\n", datatype);
	fprintf (f, "orientation=%s\n", c->orient.c_str());
	fprintf (f, "filename=%s\n", c->filename.c_str());
	fprintf (f, "endianness=%d\n", c->filebyteorder == ENDIAN_LITTLE ? 0 : 1);
	// fprintf (f, "scalefactor=%f\n", c->scalefactor ? c->scalefactor : 1.0);

	int total = c->header.size();
	int i;
	for (i = 0; i < total; i++) 
		send_userhdr (f, c->header[i]);

	fprintf (f, "data_start=%d\n", do_data ? 1 : 0);

	if (do_data) {
		if (count != fwrite (c->data,1,count,f)) {
			delete c;
			return_fifo = NULL;
			fclose (f);
#ifdef DEBUG
			fprintf(stderr, "Write failed, count=%d\n", count);
			fflush (stderr);
#endif
			return false;
		}
	}

	return_fifo = NULL;
	fflush (f);
	fclose (f);
	DEBUG_MESSAGE("Child write of 3d file info successful.")

	delete c;
	return true;
}


int
main(int argc, char **argv)
{
	if (argc != 4)
		COMPLAIN(NOARG)

	char *path = argv[1];
	char len = strlen(path);
	char *opstr = argv[3];
	char op = atoi(opstr);

	fifopath = argv[2];

	// Catch serious errors.
	if (op < 0)
		COMPLAIN(BADOP)
	if (len < 1) 
		COMPLAIN(BADPATH)

	// Ascertain file type.
	//
	// Note: If we are doing a time series, the 'path' param
	// is to be ignored because we will receive a list
	// of params in the fifo.
	//
	vector<VBFF> potentials;
	int total_potentials = 0;
	char type = TYPE_NONE;
	if (op & OP_TSRA)
		type = TYPE_TSRA;
	else {
		potentials = EligibleFileTypes(string (path));
		total_potentials = potentials.size();
		if (total_potentials) {
#ifdef DEBUG
			fprintf(stderr, "Getdata: VBFFF says type= %s, dims= %d\n",
				potentials[0].name.c_str(),
				potentials[0].dimensions);
			fflush(stderr);
#endif
			switch (potentials[0].dimensions) {
			case 1:
				type = TYPE_1D;
				break;
			case 2:
				type = TYPE_2D;
				break;
			case 3:
				type = TYPE_3D;
				break;
			case 4:
				type = TYPE_4D;
				break;
			default:
				COMPLAIN(ERROR_BADDIMS)
			}
		} else {
#ifdef DEBUG
			fprintf(stderr, "getdata: VBFF is unfamiliar\n");
			fflush(stderr);
#endif
			type = TYPE_NONE;
		}
	}
	
	int child;
	if (!(child = fork())) {
		int err;

		DEBUG_MESSAGE("Entered child process")

		// If we are only telling the file type,
		// send it and exit.
		//
		if (!op) {
			FILE *f = fopen(fifopath, "w");
			if (!f) {
				COMPLAIN(BADPATH);
			}

			string name = "UNKN";
			if (total_potentials)
				name = potentials[0].getName();

			char *s = (char*)name.c_str();
			if (s && !strncmp(s,"VoxBo ", 6))
				s += 6;
			
			fprintf (f, "dims=%d\n", type - TYPE_1D + 1);
			fprintf (f, "type=%s\n", s);
			fclose(f);
			return 0;
		}
		else
		{
			switch (type) {
			case TYPE_3D:
				if (!send_3d (path, fifopath, op)) {
					COMPLAIN(READERROR)
				}
				break;
			case TYPE_TSRA:
				if ((err = send_timeseries (fifopath, op)))
					COMPLAIN(err)
				break;
			case TYPE_4D:
				if ((err = send_4d (path, fifopath, op)))
					COMPLAIN(err)
				break;
			default:
				COMPLAIN(NOSUPPORT)
			}

			return 0;
		}
	}

	DEBUG_MESSAGE("Parent exiting.")

	return 0;
}

