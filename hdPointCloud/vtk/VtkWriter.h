#pragma once

#include <stdio.h>

class CVtkWriter
{
public:
	void set_nverts(int nverts);
	void set_nfaces(int nfaces);

	void write_vertex(const float* v_pos_f);
	void write_triangle(const int* t_idx, const bool* t_final);
	void write_triangle(const int* t_idx);
	void write_finalized(int final_idx);

	void close(bool close_file=true, bool update_header=true); // virtual base function must be reimplemented

	// functions

	bool open(FILE* file);

	CVtkWriter(void);
	~CVtkWriter(void);
private:
	FILE* file;
	int vertex_buffer_alloc;
	float* vertex_buffer;
	int triangle_buffer_alloc;
	int* triangle_buffer;
	int m_nverts;
	int m_nfaces;

	int v_count;
	int f_count;

	double* offset_d;

	float* bb_min_f;
	float* bb_max_f;
	bool need_pre_order;

};

