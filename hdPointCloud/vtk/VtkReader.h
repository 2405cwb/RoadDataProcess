#pragma once
#include <stdio.h>

// events 
typedef enum
{
	VTKIO_ERROR = -1,
	VTKIO_EOF = 0,
	VTKIO_VERTEX = 1,
	VTKIO_TRIANGLE = 2,
	VTKIO_FINALIZED = 3
} IOEvent;

class CVtkReader
{
public:
	// interface function implementations

	bool compute_bounding_box(); // reimplements virtual base function

	IOEvent read_element();
	IOEvent read_event();

	void close(bool close_file=true); // virtual base function must be reimplemented

	// functions

	bool open(FILE* file);

	// get vertices' counts
	int GetVerticesCounts() { return nverts; }

	// get faces' counts
	int GetFaceCounst() {return nfaces;}

	// get point
	inline void GetVertice(int& idx, float& posx, float& posy, float& posz){
		idx = v_idx;
		posx = v_pos_f[0];
		posy = v_pos_f[1];
		posz = v_pos_f[2];
	}

	// get face
	inline void GetFace(unsigned int& id1, unsigned int& id2,unsigned int& id3){
		id1 = t_idx[0];
		id2 = t_idx[1];
		id3 = t_idx[2];
	}

	CVtkReader(void);
	~CVtkReader(void);

private:
	int skipped_lines;
	FILE* file;
	char* line;

	bool post_order;

	// 顶点相关变量
	int v_idx;
	float v_pos_f[3];

	// 三角形变量
	int t_idx[3];
	bool t_final[3];

	float* bb_min_f;
	float* bb_max_f;

	// 空间索引化变量
	int final_idx;

	int v_count;
	int f_count;

	int nverts;
	int nfaces;

	double* offset_d;
};

