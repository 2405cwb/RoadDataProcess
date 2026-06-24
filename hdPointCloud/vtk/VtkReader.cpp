#include "StdAfx.h"
#include "VtkReader.h"

#ifdef _WIN32
#include <fcntl.h>
#include <io.h>
#endif

#include <stdlib.h>
#include <string>

#include "vec3fv.h"
#include "vec3iv.h"

#define MAX_LINE_SIZE 1024

CVtkReader::CVtkReader(void)
{
	// init 
	file = 0;
	line = 0;

	v_idx = -1;
	v_pos_f[0] = v_pos_f[1] = v_pos_f[2] = 0;

	t_idx[0] = t_idx[1] = t_idx[2] = -1;
	t_final[0] = t_final[1] = t_final[2] = false;

	final_idx = -1;

	nfaces = -1;
	nverts = -1;

	f_count = -1;
	v_count = -1;

	offset_d = 0;

	bb_min_f = 0;
	bb_max_f = 0;

	post_order = false;
}


CVtkReader::~CVtkReader(void)
{
	// clean-up 
	if (v_count != -1)
	{
		close(); // user must have forgotten to close the mesh
	}
	
	if (offset_d) delete [] offset_d;
	if (bb_min_f) delete [] bb_min_f;
	if (bb_max_f) delete [] bb_max_f;
}

bool CVtkReader::open(FILE* file)
{
	if (file == 0)
	{
		return false;
	}

#ifdef _WIN32
	if (file == stdin)
	{
		if(_setmode( _fileno( stdin ), _O_TEXT ) == -1 )
		{
			fprintf(stderr, "ERROR: cannot set stdin to text (translated) mode\n");
		}
	}
#endif

	this->file = file;

	skipped_lines = 0;
	post_order = false;

	line = (char*)malloc(sizeof(char) * MAX_LINE_SIZE);

	while (true)
	{
		if (fgets(line, sizeof(char) * MAX_LINE_SIZE, file) == 0)
		{
			free(line);
			line = 0;
			return false;
		}
		if (strncmp(line,"POINTS ",7) == 0)
		{
			if (sscanf(&(line[7]), "%d", &nverts) == 1)
			{
				break;
			}
			else
			{
				skipped_lines++;
			}
		}
	}

	// read next line
	if (fgets(line, sizeof(char) * MAX_LINE_SIZE, file) == 0)
	{
		free(line);
		line = 0;
		return false;
	}

	v_count = 0;
	f_count = 0;

	v_idx = -1;
	final_idx = -1;
	v_pos_f[0] = v_pos_f[1] = v_pos_f[2] = 0.0f;
	t_idx[0] = t_idx[1] = t_idx[2] = -1;
	t_final[0] = t_final[1] = t_final[2] = false;

	return true;
}

bool CVtkReader::compute_bounding_box()
{
	// alloc bounding box
	if (bb_min_f == 0) bb_min_f = new float[3];
	if (bb_max_f == 0) bb_max_f = new float[3];
	// read first element
	if (sscanf(&(line[0]), "%f %f %f", &(v_pos_f[0]), &(v_pos_f[1]), &(v_pos_f[2])) != 3)
	{
		fprintf(stderr, "ERROR: corrupt VTK file\n");
		exit(1);
	}
	// read next line
	if (fgets(line, sizeof(char) * 256, file) == 0)
	{
		fprintf(stderr, "WARNING: early terminated VTK file\n");
		free(line);
		line = 0;
		return true;
	}
	// init bounding box
	VecCopy3fv(bb_min_f, v_pos_f);
	VecCopy3fv(bb_max_f, v_pos_f);
	// process remaining elements
	for (v_count = 1; v_count < nverts; v_count++)
	{
		// read next element
		while (sscanf(&(line[0]), "%f %f %f", &(v_pos_f[0]), &(v_pos_f[1]), &(v_pos_f[2])) != 3)
		{
			// read next line
			if (fgets(line, sizeof(char) * 256, file) == 0)
			{
				fprintf(stderr, "WARNING: early terminated VTK file\n");
				free(line);
				line = 0;
				return true;
			}
		}
		// read next line
		if (fgets(line, sizeof(char) * 256, file) == 0)
		{
			fprintf(stderr, "WARNING: early terminated VTK file\n");
			free(line);
			line = 0;
			return true;
		}
		// update bounding box
		VecUpdateMinMax3fv(bb_min_f, bb_max_f, v_pos_f);
	}
	// we may as well read the nfaces field now
	while (true)
	{
		if (strncmp(line,"CELLS ",6) == 0)
		{
			if (sscanf(&(line[6]), "%d", &nfaces) == 1)
			{
				break;
			}
			else
			{
				skipped_lines++;
			}
		}
		if (fgets(line, sizeof(char) * MAX_LINE_SIZE, file) == 0)
		{
			free(line);
			line = 0;
			return true;
		}
	}
	// read next line
	if (fgets(line, sizeof(char) * MAX_LINE_SIZE, file) == 0)
	{
		free(line);
		line = 0;
		return true;
	}
	return true;
}

void CVtkReader::close(bool close_file)
{
	// close
	if (skipped_lines) fprintf(stderr,"WARNING: skipped %d lines.\n",skipped_lines);
	skipped_lines = 0;
	if (close_file && file && file != stdin) fclose(file);
	if (line)
	{
		free(line);
		line = 0;
	}
	// close of interface
	v_count = -1;
	f_count = -1;
}

IOEvent CVtkReader::read_element()
{
	if (line == 0)
	{
		return VTKIO_EOF;
	}

	if (v_count < nverts)
	{
		v_idx = v_count;
		while (sscanf(&(line[0]), "%f %f %f", &(v_pos_f[0]), &(v_pos_f[1]), &(v_pos_f[2])) != 3)
		{
			if (fgets(line, sizeof(char) * MAX_LINE_SIZE, file) == 0)
			{
				fprintf(stderr,"WARNING: early end-of-file after %d of %d vertices\n", v_count, nverts);
				free(line);
				line = 0;
				return VTKIO_EOF;
			}
			skipped_lines++;
		}
		v_count++;
		if (fgets(line, sizeof(char) * MAX_LINE_SIZE, file) == 0)
		{
			if (v_count < nverts || f_count < nfaces)
			{
				fprintf(stderr,"WARNING: early end-of-file after %d of %d vertices\n", v_count, nverts);
			}
			free(line);
			line = 0;
		}
		return VTKIO_VERTEX;
	}
	else if (f_count == 0)
	{
		// if we just start reading the face block we need to skip a few lines
		while (true)
		{
			if (strncmp(line,"CELLS ",6) == 0)
			{
				if (sscanf(&(line[6]), "%d", &nfaces) == 1)
				{
					break;
				}
				else
				{
					skipped_lines++;
				}
			}
			if (fgets(line, sizeof(char) * MAX_LINE_SIZE, file) == 0)
			{
				if (f_count < nfaces) fprintf(stderr,"WARNING: early end-of-file after %d of %d faces\n", f_count, nfaces);
				free(line);
				line = 0;
				return VTKIO_EOF;
			}
		}
		// read next line
		if (fgets(line, sizeof(char) * MAX_LINE_SIZE, file) == 0)
		{
			if (f_count < nfaces) fprintf(stderr,"WARNING: early end-of-file after %d of %d faces\n", f_count, nfaces);
			free(line);
			line = 0;
			return VTKIO_EOF;
		}
	}
	if (f_count < nfaces)
	{
		int deg;
		while (true)
		{
			if (sscanf(&(line[0]), "%d %d %d %d", &(deg), &(t_idx[0]), &(t_idx[1]), &(t_idx[2])) == 4)
			{
				if (deg != 3) fprintf(stderr,"WARNING: polygon %d has degree of %d\n", f_count, deg);
				f_count++;
				break;
			}
			else
			{
				skipped_lines++;
			}
			if (fgets(line, sizeof(char) * 256, file) == 0)
			{
				if (f_count < nfaces)
				{
					fprintf(stderr,"WARNING: early end-of-file after %d of %d faces\n", f_count, nfaces);
				}
				free(line);
				line = 0;
				return VTKIO_EOF;
			}
		}
		if (fgets(line, sizeof(char) * MAX_LINE_SIZE, file) == 0)
		{
			if (f_count < nfaces)
			{
				fprintf(stderr,"WARNING: early end-of-file after %d of %d faces\n", f_count, nfaces);
			}
			free(line);
			line = 0;
		}
		return VTKIO_TRIANGLE;
	}
	free(line);
	line = 0;
	return VTKIO_EOF;
}

IOEvent CVtkReader::read_event()
{
	return read_element();
}
