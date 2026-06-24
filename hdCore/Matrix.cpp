/*! @file
********************************************************************************
<PRE>
模块名       : 
文件名       : Matrix.cpp 
相关文件     : Matrix.h 
文件实现功能 : 集成系统算法：矩阵运算算法实现
作者         : 程鹏 
版本         : 1.0
--------------------------------------------------------------------------------
备注         : 
--------------------------------------------------------------------------------
修改记录 : 
日 期        版本   修改人                 修改内容 

</PRE>
********************************************************************************

* 版权所有(c) 2012, 海达数云技术有限公司, 保留所有权利

*******************************************************************************/

//////////////////////////////////////////////////////////////////////
// Matrix.cpp: implementation of the CMatrix class.
//
//////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "Matrix.h"

#include <assert.h>
#include <math.h>

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
//int CMatrix::CMatrixCount = 0;
namespace hd
{

// copy single pointer array in to the Matrix
CMatrix::CMatrix(int row, int col, double array[])
{
	rows = ( row > 0 ? row : 1 );
	cols = ( col > 0 ? col : 1 );

	ptr = new double[ rows * cols ];	// create space for CMatrix
	assert( ptr != 0 );		// terminate if memory nor allocated
//	CMatrixCount++;			// count one more object

	memcpy(ptr, array, sizeof(double)*row*col);
}

// Default constructor for class CMatrix ( default size 1,1 )
CMatrix::CMatrix(int row, int col)
{
	rows = ( row > 0 ? row : 1 );
	cols = ( col > 0 ? col : 1 );
	ptr = new double[ rows * cols ];	// create space for CMatrix
	assert( ptr != 0 );		// terminate if memory nor allocated
//	CMatrixCount++;			// count one more object
	
	memset(ptr, 0, sizeof(double)*rows*cols);
}

// Copy constructor for class CMatrix ( default size 1,1 )
CMatrix::CMatrix( const CMatrix &init ) : rows( init.rows ), cols( init.cols )
{
	ptr = new double[ rows * cols ];	// create space for CMatrix
	assert( ptr != 0 );				// terminate if memory nor allocated
//	CMatrixCount++;					// count one more object
	
	memcpy(ptr, init.ptr, sizeof(double)*rows*cols);
}

// copy a single pointer array in to the CMatrix
const CMatrix &CMatrix::copyArray( int r, int c, const double *array )
{
	CMatrix buffer( r, c );
	
	memcpy(buffer.ptr, array, sizeof(double)*r*c);
	
	*this = buffer;
	
	return *this;
}

// Destructor for class CMatrix
CMatrix::~CMatrix()
{
	delete [] ptr;	// reclaim space for CMatrix
//	CMatrixCount--;	// one fewer object
}

////////////////////////////////////////////////////
//delete if more than 1
CMatrix &CMatrix::DeleteBuffer()
{
	if(cols * rows == 1) //at least 1
		return *this;

	rows = cols = 1;
	CMatrix buffer( rows, cols);	// create the buffer CMatrix
	buffer.ptr[0]  = 0;

	*this = buffer;  // copy the buffer to this
	
	return *this;  // reference return; enables cascading.
}

////////////////////////////////////////////////////
//return the sum of all elements
double CMatrix::getMatrixSum() const
{
	double sum = 0.0;	// variable to keep the max Value

	for ( int i = 0; i < cols * rows; i ++ )
			sum += ptr[ i ];
		
	return sum;
}

// Retun the max value in the CMatrix
double CMatrix::getMaxVal() const
{
	double maxVal;	// variable to keep the max Value
	
	maxVal = ptr[ 0 ];
	
	for ( int i = 1; i < cols * rows; i ++ )
		if ( ptr[ i ] > maxVal )
			maxVal = ptr[ i ];
		
	return maxVal;
}

// Retun the max value of a row of the CMatrix
double CMatrix::getMaxRowVal( const int r ) const
{
	assert( r < rows );	// check the row number
	
	double maxVal;	// variable to keep the max Value
	
	maxVal = ptr[ r * cols ];
	
	for ( int i = r * cols; i < r * cols + cols; i ++ )
		if ( ptr[ i ] > maxVal )
			maxVal = ptr[ i ];
		
	return maxVal;
}

// Retun the max value of a column of the CMatrix
double CMatrix::getMaxColVal( const int c ) const
{
	assert( c < cols );	// check the column number
	
	double maxVal;	// variable to keep the max Value
	
	maxVal = ptr[ c ];
	
	for ( int i = c; i <= ( rows - 1 ) * cols + c; i += cols )
		if ( ptr[ i ] > maxVal )
			maxVal = ptr[ i ];
		
		return maxVal;
}

// Retun the min value in the CMatrix
double CMatrix::getMinVal() const
{
	double minVal;	// variable to keep the min Value
	
	minVal = ptr[ 0 ];
	
	for ( int i = 1; i < cols * rows; i ++ )
		if ( ptr[ i ] < minVal )
			minVal = ptr[ i ];
		
		return minVal;
}

// Retun the min value of a row of the CMatrix
double CMatrix::getMinRowVal( const int r ) const
{
	assert( r < rows );	// check the row number
	
	double minVal;	// variable to keep the min Value
	
	minVal = ptr[ r * cols ];
	
	for ( int i = r * cols; i < r * cols + cols; i ++ )
		if ( ptr[ i ] < minVal )
			minVal = ptr[ i ];
		
		return minVal;
}

// Retun the min value of a column of the CMatrix
double CMatrix::getMinColVal( const int c ) const
{
	assert( c < cols );	// check the column number
	
	double minVal;	// variable to keep the min Value
	
	minVal = ptr[ c ];
	
	for ( int i = c; i <= ( rows - 1 ) * cols + c; i += cols )
		if ( ptr[ i ] < minVal )
			minVal = ptr[ i ];
		
		return minVal;
}

// Retun the mean of the values in the CMatrix
double CMatrix::getMean() const
{
	double total = 0;	// variable to keep the total Value
	
	for ( int i = 1; i < cols * rows; i ++ )
		total += ptr[ i ];
	
	return ( total / ( cols * rows ) );
}

// Retun the mean of the values in the CMatrix
double CMatrix::getAbsMean() const
{
	double total = 0;	// variable to keep the total Value
	
	for ( int i = 1; i < cols * rows; i ++ )
		total += fabs(ptr[ i ]);
	
	return ( total / ( cols * rows ) );
}

// Retun the mean value of a row of the CMatrix
void CMatrix::getRow(CMatrix& mtxR, const int r ) const
{
	assert( r < rows );	// check the row number
	mtxR.Init(1, cols);
	
	memcpy(mtxR.ptr, ptr+r*cols, sizeof(double)*cols);

	return;
}

// Retun the mean value of a row of the CMatrix
void CMatrix::getCol(CMatrix& mtxC, const int c ) const
{
	assert( c < cols );	// check the row number

	mtxC.Init(rows, 1);	
	
	for(int i=0; i<rows; i++) 
		mtxC(i, 0) = ptr[i*cols+c];

	return;
}

// Retun the mean value of a row of the CMatrix
double CMatrix::getRowMean( const int r ) const
{
	assert( r < rows );	// check the row number
	
	double total = 0;	// variable to keep the total Value
	
	for ( int i = r * cols; i < r * cols + cols; i ++ )
		total += ptr[ i ];
	
	return ( total / cols );
}

// Retun the mean value of a row of the CMatrix
double CMatrix::getRowAbsMean( const int r ) const
{
	assert( r < rows );	// check the row number
	
	double total = 0;	// variable to keep the total Value
	
	for ( int i = r * cols; i < r * cols + cols; i ++ )
		total += fabs(ptr[ i ]);
	
	return ( total / cols );
}

// Retun the mean value of a column of the CMatrix
double CMatrix::getColMean( const int c ) const
{
	assert( c < cols );	// check the column number
	
	double total = 0;	// variable to keep the total Value
	
	for ( int i = c; i <= ( rows - 1 ) * cols + c; i += cols )
		total += ptr[ i ];
	
	return ( total / rows );
}

// Retun the mean value of a column of the CMatrix
double CMatrix::getColAbsMean( const int c ) const
{
	assert( c < cols );	// check the column number
	
	double total = 0;	// variable to keep the total Value
	
	for ( int i = c; i <= ( rows - 1 ) * cols + c; i += cols )
		total += fabs(ptr[ i ]);
	
	return ( total / rows );
}

// find the value in the CMatrix and return as a nx2 CMatrix
const CMatrix CMatrix::findVal( const double v ) const
{
	CMatrix buffer( 1, 2 );
	CMatrix temp( 1, 2 );
	
	for ( int i = 0; i < cols * rows; i++)
	{
		if ( ptr[ i ] == v )
		{
			temp(0, 0 ) = i / cols;
			temp(0, 1 ) = i % cols;
			buffer.insertRow( buffer.rows - 1, temp );
		}
	}
	
	buffer.deleteRow( buffer.rows - 1 ); // delete the last row 0 0.
	
	return buffer;	// return as nx2 CMatrix
}


// Return the number of value in the CMatrix
int CMatrix::getValCount( const double v ) const
{
	int count = 0;
	
	for ( int i = 0; i < cols * rows; i++ )
		if ( ptr[ i ] == v )
			count++;
		
		return count;
}

////////////////////////////////////////////////////////
// Overloaded assignment operator
// const return avoids ( a1 = a2 ) = a3
const CMatrix &CMatrix::operator =( const CMatrix &right )
{
	if ( &right != this ) {	//check for self assignment
		// for arrays of different sizes, deallocate original
		// left side array, then allocate new left side array.
		if ( cols != right.cols  || rows != right.rows ) {
			delete [] ptr;					// reclaim space;
			rows = right.rows;				// resize the object
			cols = right.cols;				// resize the object
			ptr = new double[ rows * cols ];	// create space for CMatrix copy
			assert( ptr != 0 );				// terminate if not allocated
		}

		// copy array into object
		memcpy(ptr, right.ptr, sizeof(double)*rows*cols);
	}
	return *this;	// enables x = y = z;
}

// Overloaded assignment operator
// const return avoids ( a1 = a2 ) = a3
const CMatrix &CMatrix::operator =( const char ch )
{
	assert( ch == 'I' && isSquare() );
	
	if ( ch == 'I' )
		for ( int i = 0; i < rows * cols; i++ )
			if ( i / cols == i % cols )	
				ptr[ i ] = 1;		// put 1's in the diagonals
			else
				ptr[ i ] = 0;		// put 0's in the other cells
			
	return *this;	// enables x = y = z;
}

// Determine if two arrays are equal and
// return true, otherwise return false.
bool CMatrix::operator ==( const CMatrix &right ) const
{
	if ( cols != right.cols  || rows != right.rows )
		return false;			// CMatrix's of different sizes
	
	for ( int i = 0; i < rows * cols; i++ )
		if ( ptr[ i ] != right.ptr[ i ] )
			return false;		// arrays are not equal
		
	return true;				// arrays are equal
}

// Overloaded subscript operator for non-const CMatrix
// reference return creates an lvalue
double &CMatrix::operator ()( int r, int c )
{
	// check for subscript out of range error
	assert( 0 <= r && r < rows && 0 <= c && c < cols );
	
	return ptr[ r * cols + c ];	// reference return
}

// Overloaded subscript operator for const CMatrix
// const reference return creates an rvalue
const double &CMatrix::operator ()( int r, int c ) const
{
	// check for subscript out of range error
	assert( 0 <= r && r <= rows && 0 <= c && c <= cols );
	
	return ptr[ r  * cols + c ];	// const reference return
}

// Overloaded addition operator
// add a fixed value to the CMatrix
CMatrix CMatrix::operator +( const double value )
{
	CMatrix buffer( rows, cols ); // create a temp CMatrix object not to change this
	
	for ( int i = 0; i < rows * cols; i++ )
		buffer.ptr[ i ] = ptr[ i ] + value;	// add value to all member of the CMatrix
	
	// return temporary object not to change this
	return buffer;			// value return; not a reference return
}

// Overloaded addition operator
// add two CMatrixs's
CMatrix CMatrix::operator +( const CMatrix &right )
{
	CMatrix buffer( rows, cols );	// create a temp CMatrix object not to change this
	
	// terminate if matrices are not in the same size
	assert( cols == right.cols && rows == right.rows );
	
	for ( int i = 0; i < rows * cols; i++ )
		buffer.ptr[ i ] = ptr[ i ] + right.ptr[ i ];
	
	// return temporary object not to change this
	return buffer;			// value return; not a reference return
}

// Overloaded addition operator
// add a fixed value to the CMatrix modify CMatrix
void CMatrix::operator +=( const double value )
{
	for ( int i = 0; i < rows * cols; i++ )
		ptr[ i ] = ptr[ i ] + value;	// add value to all member of the CMatrix
	
}

// Overloaded addition operator
// add two CMatrixs's modify this CMatrix
void CMatrix::operator +=( const CMatrix &right )
{
	// terminate if matrices are not in the same size
	assert( cols == right.cols && rows == right.rows );
	
	for ( int i = 0; i < rows * cols; i++ )
		ptr[ i ] = ptr[ i ] + right.ptr[ i ];
	
}

// Overloaded substraction operator
// substract a fixed value from a CMatrix
CMatrix CMatrix::operator -( const double value )
{
	CMatrix buffer( rows, cols ); // create a temp CMatrix object not to change this
	
	for ( int i = 0; i < rows * cols; i++ )
		buffer.ptr[ i ] = ptr[ i ] - value;	// substract a value from all member of the CMatrix
	
	// return temporary object not to change this
	return buffer;			// value return; not a reference return
}

// Overloaded substraction operator
// substract a CMatrix right from this
CMatrix CMatrix::operator -( const CMatrix &right )
{
	CMatrix buffer( rows, cols );	// create a temp CMatrix object not to change this
	
	// terminate if matrices are not in the same size
	assert( cols == right.cols && rows == right.rows );
	
	for ( int i = 0; i < rows * cols; i++ )
		buffer.ptr[ i ] = ptr[ i ] - right.ptr[ i ];
	
	// return temporary object not to change this
	return buffer;			// value return; not a reference return
}

// Overloaded substraction operator
// substract a fixed value from a CMatrix modify this CMatrix
void CMatrix::operator -=( const double value )
{
	for ( int i = 0; i < rows * cols; i++ )
		ptr[ i ] = ptr[ i ] - value;	// substract a value from all member of the CMatrix
	
}

// Overloaded substraction operator
// substract a CMatrix right from this modify this CMatrix
void CMatrix::operator -=( const CMatrix &right )
{
	// terminate if matrices are not in the same size
	assert( cols == right.cols && rows == right.rows );
	
	for ( int i = 0; i < rows * cols; i++ )
		ptr[ i ] = ptr[ i ] - right.ptr[ i ];
	
}

// Overloaded multiplication operator
// Multiply a CMatrix with a value
CMatrix CMatrix::operator *( const double value )
{
	CMatrix buffer( rows, cols );	// create a temp CMatrix object not to change this
	
	for ( int i = 0; i < rows * cols; i++ )
		buffer.ptr[ i ] = ptr[ i ] * value;
	
	// return temporary object not to change this
	return buffer;			// value return; not a reference return
}

// Overloaded multiplicatipn operator
// Multiply two matrices 
CMatrix CMatrix::operator *( const CMatrix &right )
{
	// terminate if matrices are not in the format: (m x n), (n x k)
	assert( cols == right.rows );

	CMatrix buffer( rows, right.cols );  // a CMatrix of (m x k)
	double temp;

	for ( int i = 0; i < rows; i++ )
	{
		for ( int k = 0; k < right.cols; k++ )
		{
			temp = 0;
			for ( int j = 0; j < cols; j++ )
			{
				temp += ptr[ i * cols + j ] * right.ptr[ j * right.cols + k ];
			}
			buffer.ptr[ i * right.cols + k ] = temp; 
		}
	}
	
	return buffer;
}

// Overloaded multiplication operator
// Multiply a CMatrix with a value modify this CMatrix
void CMatrix::operator *=( const double value )
{
	for ( int i = 0; i < rows * cols; i++ )
		ptr[ i ] = ptr[ i ] * value;
	
}

// Overloaded multiplicatipn operator
// Multiply two matrices modify this CMatrix	  ( ! SLOWER THAN * ! )
void CMatrix::operator *=( const CMatrix &right )
{
	// terminate if matrices are not in the format: (m x n), (n x k)
	assert( cols == right.rows );

	CMatrix buffer( rows, right.cols );  // a CMatrix of (m x k)
	double temp;
	
	for ( int i = 0; i < rows; i++ ) 
	{
		for ( int k = 0; k < right.cols; k++ ) 
		{
			temp = 0;
			for ( int j = 0; j < cols; j++ ) 
			{
				temp += ptr[ i * cols + j ] * right.ptr[ j * right.cols + k ];
			}
			buffer.ptr[ i * right.cols + k ] = temp; 
		}
	}
	
	*this = buffer;	// using a buffer to calculate the result
	//then copy buffer to this
}

// Overloaded power operator
// Take the value th power of the CMatrix
CMatrix CMatrix::operator ^( const int value )
{
	assert( isSquare() && value > 0 );
	
	CMatrix buffer( rows, cols );	// a CMatrix( rows, cols )
	
	buffer = *this;
	
	for ( int i = 1; i < value; i++ )
		buffer = buffer * *this;
		
	return buffer;
}

// Overloaded division operator
// Divide the CMatrix by value
CMatrix CMatrix::operator /( const double value )
{
	assert( value != 0 );
	
	CMatrix buffer( rows, cols );	// a CMatrix( rows, cols )
	
	for ( int i = 0; i < rows * cols; i++ )
		buffer.ptr[ i ] = ptr[ i ] / value;
	
	// return temporary object not to change this
	return buffer;			// value return; not a reference return
}

// Overloaded division operator
// Divide the CMatrix by value
void CMatrix::operator /=( const double value )
{
	assert( value != 0 );
	
	for ( int i = 0; i < rows * cols; i++ )
		ptr[ i ] = ptr[ i ] / value;
	
}

///////////////////////////////////////////////////////
// convert CMatrix to identity CMatrix of the same size
const CMatrix &CMatrix::convToIdentity()
{
	assert( isSquare() );
	
	for ( int i = 0; i < rows * cols; i++ )
		if ( i / cols == i % cols )	
			ptr[ i ] = 1;		// put 1's in the diagonals
		else
			ptr[ i ] = 0;		// put 0's in the other cells
		
	return *this;	// enables x = y = z;
}

// convert CMatrix to inverse diagonal
const CMatrix &CMatrix::convToInvDiagonal()
{
	assert( isSquare() );
	
	for ( int i = 0; i < rows * cols; i++ )
		if ( ( i / cols ) + ( i % cols ) + 1 == cols )	
			ptr[ i ] = 1;		// put 1's in the inv diagonals
		else
			ptr[ i ] = 0;		// put 0's in the other cells
		
	return *this;	// enables x = y = z;
}

// Return the transpose of a CMatrix
// modifying the original CMatrix
CMatrix &CMatrix::getTranspose() 
{
	CMatrix buffer( cols, rows );	// create the buffer CMatrix
	
	for(int i=0; i<cols; i++)
		for(int j=0; j<rows; j++)
		{
			buffer.ptr[i*rows+j] = ptr[j*cols+i];
		}

	*this = buffer;

	return *this;
}

// calculate the LUD of the CMatrix
void CMatrix::LUBKS( int phy_size, int *outvect, double *output )
{
	int	i, j, ii, ll;
    double	sum;
	
    ii = -1;  /*when ii is set to a value >= 0 it is an index to
	the first nonvanishing element of the output*/

    for ( i = 0; i < rows; i++ ) {
		ll = outvect[ i ];
		sum = output[ ll ];
		output[ ll ] = output[ i ];
		if ( ii != -1 ) {
			for ( j = ii; j < i; j++ )
				sum = sum - ( ptr[ i * cols + j ] * output[ j ] );
		}
		else if ( sum != 0. ) {
			ii = i ; 
		}
		output[ i ] = sum;
    }
    for ( i = ( rows - 1 ); i > -1; i-- ) {
		sum = output[ i ];
		if ( i < ( rows - 1 ) ) {
			for ( j = ( i + 1 ); j < rows; j++ )
				sum = sum - ( ptr[ i * cols + j ] * output[ j ] );
		}
		output[ i ] = sum / ptr[ i * cols + i ];
    }
	
}

// calculate the LUD of the CMatrix
int CMatrix::LUD( int phy_size, int *outvect, int output )
{
	int	    i, j, k, imax; 
    double  *VV =  new double[1000 * sizeof(double)];
	double aamax, sum, dnum; 
	double SMALL = 1.0e-20;

	// allocate space for VV; and check if space allocated
 //   assert( ( VV =  new double[1000 * sizeof(double)] ) != 0 );
	
    output = 1; 	     /* No row interchanges yet*/
    
    for ( i = 0; i < rows; i++ ) {
		aamax = 0.0;
		for ( j = 0; j < rows; j++ ) {
			if( fabs( ptr[ i * cols + j ] ) > aamax )
				aamax = fabs( ptr[ i * cols + j ] );
		}
		if ( aamax == 0. ) {
			return 1; /* LUD_ERR flag Singular CMatrix */
		}
		VV[ i ] = 1.0 / aamax;
    }
    
    for ( j = 0; j < rows; j++ ) {
		if ( j > 0 ) {
			for ( i = 0; i < j; i++ ) {
				sum = ptr[ i * cols + j ];
				if ( i > 0 ) {
					for ( k = 0; k < i; k++ )
						sum = sum - ( ptr[ i * cols + k ] * ptr[ k * cols + j ] );
					ptr[ i * cols + j ] = sum;
				}
			}
		}

		aamax = 0.;

		for ( i = j; i < rows; i++ ) {
			sum = ptr[ i * cols + j ];
			if ( j > 0 ) {
				for ( k = 0; k < j; k++ )
					sum = sum - ( ptr[ i * cols + k ] * ptr[ k * cols + j ] );
				ptr[ i * cols + j ] = sum;
			}
			dnum = VV[ i ] * fabs( sum );
			if( dnum >= aamax ) {
				imax = i;
				aamax = dnum;
			}
		}
		if ( j != imax ) {
			for ( k = 0; k < rows; k++ ) {
				dnum = ptr[ imax * cols + k ];
				ptr[ imax * cols + k ] = ptr[ j * cols + k ];
				ptr[ j * cols + k ] = dnum;
			}
			output = -output;
			VV[ imax ] = VV[ j ];
		}
		outvect[ j ] = imax;
		if ( j != ( rows - 1 ) ) {
			if ( ptr[ j * cols + j ] == 0. )
				ptr[ j * cols + j ] = SMALL;
			dnum = 1.0 / ptr[ j * cols + j ];
			for ( i = ( j + 1 ); i < rows; i++ )
				ptr[ i * cols + j ] = ptr[ i * cols + j ] * dnum;
		}
    }
    if ( ptr[ ( rows - 1 ) * cols + ( rows - 1 ) ] == 0. )
		ptr[ ( rows - 1 ) * cols + ( rows - 1 ) ] = SMALL;

    delete [] VV;

    return 0; /* normal return */
}

// get the inverse of the CMatrix
CMatrix &CMatrix::getInverse()
{
	// check if the CMatrix is a square CMatrix
	assert( isSquare() );

	double *temp = new double[rows * sizeof(double)];
    int    *IND =  new int[rows * sizeof(int)];
	int D = 0, i, j;

	// check if space allocated for temp.
//	assert ( (temp = new double[rows * sizeof(double)]) != 0 );
	// check if space allocated for IND.
//	assert ( (IND =  new int[rows * sizeof(int)]) != 0 );
	
	// create the buffer CMatrix
	CMatrix buffer( rows, cols );
	CMatrix result( rows, cols );

	buffer = *this;

	if ( buffer.LUD( rows, IND, D ) == 1 )
	{
		delete [] temp;
		delete [] IND;
		assert( !buffer.LUD( rows, IND, D ) );
	}
    for ( j = 0; j < buffer.rows; j++ )//bakiniz
	{
		for ( i = 0; i < buffer.rows; i++ )
			temp[i] = 0.0;
		temp[j] = 1.0;

		buffer.LUBKS( rows, IND, temp);

		for ( i = 0; i < buffer.rows; i++ )
			result.ptr[ i * cols + j ] = temp[ i ];
    }

    delete [] temp;
	delete [] IND;

	*this = result;

	return *this;
}

// Return a sub CMatrix of a CMatrix
// without modifying the original CMatrix
CMatrix CMatrix::getSubCMatrix( const int startRow, const int startCol,
							const int rowSize, const int colSize ) const
{
	// check if really the subCMatrix is in the CMatrix
	assert( 0 <= startRow && startRow + rowSize < rows
		&& 0 <= startCol && startCol + colSize < cols );
	
	CMatrix buffer( rowSize + 1, colSize + 1 );	// create the empty subCMatrix
	
	for ( int i = startRow;	i <= startRow + rowSize; i++ )
		for ( int j = startCol; j <=startCol + colSize; j++)
			buffer.ptr[ ( i - startRow ) * ( colSize + 1 ) + j - startCol ] =
			ptr [ i * cols + j ];
		
	return buffer;
}

// Delete a row from the CMatrix
CMatrix &CMatrix::deleteRow( const int r )
{
	// check if r is a valid row number
	assert( 0 <= r && r < rows );
	
	int i;
	
	CMatrix buffer( rows - 1, cols );	// create the buffer CMatrix
	
	for ( i = 0; i / cols < r ; i++ )
		buffer.ptr[ i ] = ptr[ i ];
	for ( i = ( r + 1 ) * cols; i < rows * cols; i++ )
		buffer.ptr[ i - cols ] = ptr[ i ];
	
	*this = buffer;  // copy the buffer to this
	
	return *this;  // reference return; enables cascading.
}

// Delete a coloumn from the CMatrix
CMatrix &CMatrix::deleteCol( const int c )
{
	// check if c is a valid coloumn number
	assert( 0 <= c && c < cols );
	
	int i, j;
	
	CMatrix buffer( rows, cols - 1 );	// create the buffer CMatrix
	
	for ( j = 0; j < c; j++ ) {
		for ( i = 0; i / cols < rows; i += cols )
			buffer.ptr[ ( i / cols ) * ( cols - 1 ) + j ] = 
			ptr[ i + j ];
	}
	
	for ( j = c + 1; j < cols; j++ ) {
		for (i = 0; i / cols < rows; i += cols )
			buffer.ptr[ ( i / cols ) * ( cols - 1) + j - 1 ] = 
			ptr [ i + j ];  
	}
	
	*this = buffer;  // copy the buffer to this
	
	return *this;  // reference return; enables cascading.
}

// inserts the row CMatrix to the given row shifting other rows down
CMatrix &CMatrix::insertRow( const int r, const CMatrix &rVector ) 
{
	// check if rVector is a 1xn CMatrix
	assert( rVector.rows == 1 && rVector.cols == cols && r < rows );
	
	CMatrix buffer( rows + 1, cols );	// create the buffer CMatrix
	
	int i;
	
	// first copying the rows till r...	
	for ( i = 0; i / cols < r ; i++ )
		buffer.ptr[ i ] = ptr[ i ];
	
	// now copying the rVector into the rth row..
	for ( i = 0; i < cols; i ++)
		buffer.ptr[ r * cols + i ] = rVector.ptr[ i ];
	
	// copying the rows after r...
	for ( i = r * cols; i < rows * cols; i++ )
		buffer.ptr[ i + cols ] = ptr[ i ];
	
	*this = buffer;  // copy the buffer to this
	
	return *this;  // reference return; enables cascading.
}

// inserts the column CMatrix to the given column shifting other columns right
CMatrix &CMatrix::insertCol( const int c, const CMatrix &cVector )
{
	// check if cVector is a nx1 CMatrix
	assert( cVector.cols == 1 && cVector.rows == rows && c < cols );
	
	CMatrix buffer( rows, cols + 1 );	// create the buffer CMatrix
	int i, j;
	
	for ( i = 0; i < c; i++ ) {
		for ( j = 0; j < rows; j++ )
			buffer.ptr[ j * ( cols + 1 ) + i ] = ptr[ j * cols + i ];
	}
	
	for ( j = 0; j < rows; j++ )
		buffer.ptr[ j * ( cols + 1 ) + c ] = cVector.ptr[ j ];
	
	for ( i = c + 1; i < cols + 1; i++ ) {
		for ( j = 0; j < rows; j++ )
			buffer.ptr[ j * ( cols + 1 ) + i ] = ptr [ j * cols + i - 1 ];
	}
	
	
	*this = buffer;  // copy the buffer to this
	
	return *this;  // reference return; enables cascading.
}

// interchange rows in the CMatrix ( r1 <-> r2 )
CMatrix &CMatrix::interchangeRows( const int r1, const int r2 )
{
	// check if r1 and r2 row numbers are valid.
	assert( 0 <= r1 && r1 < rows && 0 <= r2 && r2 < rows );
	
	int rUp, rDown, i;
	
	CMatrix buffer( rows, cols );
	
	if ( r1 != r2 ) {
		// check which value is bigger.
		if ( r1 > r2 ) {
			rUp = r1;
			rDown = r2;
		}
		else {
			rUp = r2;
			rDown = r1;
		}							   		
		for ( i = 0; i / cols < rDown; i++ )
			buffer.ptr[ i ] = ptr[ i ];
		for ( i = rUp * cols; i < rUp * cols + cols; i++ )
			buffer.ptr[ i - ( ( rUp - rDown ) * cols ) ] = ptr[ i ];
		for ( i = ( rDown + 1 ) * cols; i < rUp * cols; i ++ )
			buffer.ptr[ i ] = ptr[ i ];
		for ( i = rDown * cols; i < rDown * cols + cols; i++ )
			buffer.ptr[ i + ( ( rUp - rDown ) * cols ) ] = ptr[ i ];
		for ( i = ( rUp + 1 ) * cols; i < rows * cols; i++ )
			buffer.ptr[ i ] = ptr[ i ];
	}
	
	*this = buffer;	// copy the buffer to this
	
	return *this;
}

// interchange columns in the CMatrix ( c1 <-> c2 )
CMatrix &CMatrix::interchangeCols( const int c1, const int c2 )
{
	// check if r1 and r2 row numbers are valid.
	assert( 0 <= c1 && c1 < cols && 0 <= c2 && c2 < cols );
	
	int cUp, cDown;
	
	CMatrix buffer( rows, cols );
	
	if ( c1 != c2 ) {
		// check which value is bigger.
		if ( c1 > c2 ) {
			cUp = c1;
			cDown = c2;
		}
		else {
			cUp = c2;
			cDown = c1;
		}
		
		int i, j;
		
		for ( i = 0; i < cDown; i++ ) {
			for ( j = 0; j < rows; j++ )
				buffer.ptr[ j * cols + i ] = ptr[ j * cols + i ];
		}
		
		for ( j = 0; j < rows; j++ )
			buffer.ptr[ j * cols + cDown ] = ptr[ j * cols + cUp ];
		
		for ( i = cDown + 1; i < cUp; i++ ) {
			for ( j = 0; j < rows; j++ )
				buffer.ptr[ j * cols + i ] = ptr [ j * cols + i ];
		}

		for ( j = 0; j < rows; j++ )
			buffer.ptr[ j * cols + cUp ] = ptr[ j * cols + cDown ];

		for ( i = cUp + 1; i < cols; i++ ) {
			for ( j = 0; j < rows; j++ )
				buffer.ptr[ j * cols + i ] = ptr [ j * cols + i ];
		}
	}
	
	*this = buffer;	// copy the buffer to this
	
	return *this;
	
}

// add the right CMatrix on top of this
CMatrix &CMatrix::attachTop( const CMatrix &right )
{
	// check if both matrices have the same number of columns
	assert( right.cols == cols );
	
	// create the buffer CMatrix of needed size
	CMatrix buffer( rows + right.rows, cols );
	
	for ( int i = 0; i < right.rows * right.cols; i++ )
		buffer.ptr[ i ] = right.ptr[ i ];
	for ( int j = right.rows * right.cols; j < ( rows + right.rows ) * cols; j++ )
		buffer.ptr[ j ] = ptr[ j - ( right.rows * right.cols ) ];
	
	*this = buffer;
	
	return *this;	// enable cascading
}

// add the right CMatrix under this
CMatrix &CMatrix::attachBottom( const CMatrix &right )
{
	// check if both matrices have the same number of columns
	assert( right.cols == cols );
	
	// create the buffer CMatrix of needed size
	CMatrix buffer( rows + right.rows, cols );
	
	for ( int i = 0; i < rows * cols; i++ )
		buffer.ptr[ i ] = ptr[ i ];
	for ( int j = rows * cols ; j < ( rows + right.rows ) * cols; j++ )
		buffer.ptr[ j ] = right.ptr[ j - ( rows * cols )];
	
	*this = buffer;
	
	return *this;	// enable cascading
}

// add the right CMatrix to the left of this
CMatrix &CMatrix::attachLeft( const CMatrix &right )
{
	// check if both matrices have the same number of rows
	assert( right.rows == rows );
	
	// create the buffer CMatrix of needed size
	CMatrix buffer( rows, cols + right.cols );
	int i,j;
	
	// First copying the right CMatrix to the buffer
	for ( i = 0; i < right.cols; i++ ) {
		for ( j = 0; j < right.rows; j++ )
			buffer.ptr[ j * ( cols + right.cols ) + i ] = 
			right.ptr[ j * right.cols + i ];
	}
	
	// copying the this CMatrix to the right side of the buffer.
	for ( i = right.cols; i < cols + right.cols; i++ ) {
		for ( j = 0; j < rows; j++ )
			buffer.ptr[ j * ( cols + right.cols ) + i ] = 
			ptr[ j * cols + i - right.cols ];
	}
	
	*this = buffer;
	
	return *this;	// enable cascading
}

// add the right CMatrix to the right of this
CMatrix &CMatrix::attachRight( const CMatrix &right )
{
	// check if both matrices have the same number of rows
	assert( right.rows == rows );
	
	// create the buffer CMatrix of needed size
	CMatrix buffer( rows, cols + right.cols );
	int i,j;
	
	// First copying the this CMatrix to the left side of the buffer
	for ( i = 0; i < cols; i++ ) {
		for ( j = 0; j < rows; j++ )
			buffer.ptr[ j * ( cols + right.cols ) + i ] = 
			ptr[ j * cols + i ];
	}
	
	// copying the this CMatrix to the left side of the buffer.
	for ( i = cols; i < cols + right.cols; i++ ) {
		for ( j = 0; j < right.rows; j++ )
			buffer.ptr[ j * ( cols + right.cols ) + i ] = 
			right.ptr[ j * right.cols + i - cols ];
	}
	
	*this = buffer;
	
	return *this;	// enable cascading
}

///////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
// 复矩阵的乘法
//
// 参数：
// 1. const CMatrix& AR - 左边复矩阵的实部矩阵
// 2. const CMatrix& AI - 左边复矩阵的虚部矩阵
// 3. const CMatrix& BR - 右边复矩阵的实部矩阵
// 4. const CMatrix& BI - 右边复矩阵的虚部矩阵
// 5. CMatrix& CR - 乘积复矩阵的实部矩阵
// 6. CMatrix& CI - 乘积复矩阵的虚部矩阵
//
// 返回值：BOOL型，复矩阵乘法是否成功
//////////////////////////////////////////////////////////////////////
void CMatrix::CMul(const CMatrix& AR, 
				   const CMatrix& AI, 
				   const CMatrix& BR, 
				   const CMatrix& BI,
				   CMatrix& CR, 
				   CMatrix& CI) const
{
	// 首先检查行列数是否符合要求
	assert(AR.cols == AI.cols &&
		AR.rows == AI.rows &&
		BR.cols == BI.cols &&
		BR.rows == BI.rows &&
		AR.cols == BR.rows);

	// 构造乘积矩阵实部矩阵和虚部矩阵
	CMatrix buffer_r( AR.rows, BR.cols );  // a CMatrix of (m x k)
	CMatrix buffer_i( AR.rows, BR.cols );  // a CMatrix of (m x k)

	// 复矩阵相乘
	double vr, vi, p, q, s;
    for (int i=0; i<AR.rows; ++i)
	{
	    for (int j=0; j<BR.cols; ++j)
		{
			vr = 0;	vi = 0;
            for (int k =0; k<AR.cols; ++k)
			{
                p = AR(i, k) * BR(k, j);
                q = AI(i, k) * BI(k, j);
                s = (AR(i, k) + AI(i, k)) * (BR(k, j) + BI(k, j));
                vr += p - q;
                vi += s - p - q;
			}
            buffer_r(i, j) = vr;
            buffer_i(i, j) = vi;
        }
	}

	CR = buffer_r;
	CI = buffer_i;
}

//////////////////////////////////////////////////////////////////////
// 对称正定矩阵的乔里斯基分解与行列式的求值
//
// 参数：
// 1. double* dblDet - 返回行列式的值
//
// 返回值：BOOL型，求解是否成功
//////////////////////////////////////////////////////////////////////
void CMatrix::DetCholesky(double* dblDet)
{ 
	// 不满足求解要求
	assert (ptr[0] > 0.0);
	
	int u,l;
    double d;

	// 乔里斯基分解
    ptr[0]=sqrt(ptr[0]);
    d=ptr[0];

    for (int i=1; i<=cols-1; i++)
    { 
		u=i*cols; 
		ptr[u]=ptr[u]/ptr[0];
	}
    
	for (int j=1; j<=cols-1; j++)
    { 
		l=j*cols+j;
        for (int k=0; k<=j-1; k++)
        { 
			u=j*cols+k; 
			ptr[l]=ptr[l]-ptr[u]*ptr[u];
		}
        
		// 不满足求解要求
		assert(ptr[l] > 0.0);

        ptr[l]=sqrt(ptr[l]);
        d=d*ptr[l];
        
		for (int i=j+1; i<=cols-1; i++)
        { 
			u=i*cols+j;
            for (int k=0; k<=j-1; k++)
				ptr[u]=ptr[u]-ptr[i*cols+k]*ptr[j*cols+k];
            
			ptr[u]=ptr[u]/ptr[l];
        }
    }
    
	// 行列式求值
	*dblDet=d*d;
    
	// 下三角矩阵
    for (int i=0; i<=cols-2; i++)
		for (int j=i+1; j<=cols-1; j++)
			ptr[i*cols+j]=0.0;
}

//////////////////////////////////////////////////////////////////////
// 求行列式值的全选主元高斯消去法
//
// 参数：无
//
// 返回值：double型，行列式的值
//////////////////////////////////////////////////////////////////////
double CMatrix::DetGauss()
{ 
	int i, j, k, is,js,l,u,v;
    double f,det,q,d;
    
	// 初值
	f=1.0; 
	det=1.0;
    
	// 消元
	for ( k=0; k<=cols-2; k++)
    { 
		q=0.0;
        for ( i=k; i<=cols-1; i++)
		{
			for ( j=k; j<=cols-1; j++)
			{ 
				l=i*cols+j; 
				d=fabs(ptr[l]);
				if (d>q) 
				{ 
					q=d; 
					is=i; 
					js=j;
				}
			}
		}

        if (q == 0.0)
        { 
			det=0.0; 
			return(det);
		}
        
		if (is!=k)
        { 
			f=-f;
            for ( j=k; j<=cols-1; j++)
            { 
				u=k*cols+j; 
				v=is*cols+j;
                d=ptr[u]; 
				ptr[u]=ptr[v]; 
				ptr[v]=d;
            }
        }
        
		if (js!=k)
        { 
			f=-f;
            for (i=k; i<=cols-1; i++)
            {
				u=i*cols+js; 
				v=i*cols+k;
                d=ptr[u]; 
				ptr[u]=ptr[v]; 
				ptr[v]=d;
            }
        }

        l=k*cols+k;
        det=det*ptr[l];
        for (i=k+1; i<=cols-1; i++)
        { 
			d=ptr[i*cols+k]/ptr[l];
            for (j=k+1; j<=cols-1; j++)
            { 
				u=i*cols+j;
                ptr[u]=ptr[u]-d*ptr[k*cols+j];
            }
        }
    }
    
	// 求值
	det=f*det*ptr[cols*cols-1];

    return(det);
}

//////////////////////////////////////////////////////////////////////
// 获取指定行的向量
//
// 参数：
// 1. int nRows - 指定的矩阵行数
// 2.  double* pVector - 指向向量中各元素的缓冲区
//
// 返回值：int 型，向量中元素的个数，即矩阵的列数
//////////////////////////////////////////////////////////////////////
int CMatrix::GetRowVector(int nRow, double* pVector) const
{
	assert( nRow < rows );

	if (pVector == 0)
		delete pVector;

	pVector = new double[cols];
	assert(pVector != 0);

	for (int j=0; j<cols; ++j)
		pVector[j] = ptr[nRow*cols+j];

	return cols;
}

//////////////////////////////////////////////////////////////////////
// 获取指定列的向量
//
// 参数：
// 1. int nCols - 指定的矩阵列数
// 2.  double* pVector - 指向向量中各元素的缓冲区
//
// 返回值：int 型，向量中元素的个数，即矩阵的行数
//////////////////////////////////////////////////////////////////////
int CMatrix::GetColVector(int nCol, double* pVector) const
{
	assert( nCol < cols );

	if (pVector == 0)
		delete pVector;

	pVector = new double[rows];
	assert(pVector != 0);

	for (int i=0; i<rows; ++i)
		pVector[i] = ptr[i*cols+nCol];

	return rows;
}

//////////////////////////////////////////////////////////////////////
// 求广义逆的奇异值分解法，分解成功后，原矩阵对角线元素就是矩阵的奇异值
//
// 参数：
// 1. CMatrix& mtxAP - 返回原矩阵的广义逆矩阵
// 2. CMatrix& mtxU - 返回分解后的U矩阵
// 3. CMatrix& mtxV - 返回分解后的V矩阵
// 4. double eps - 计算精度，默认值为0.000001
//
// 返回值：BOOL型，求解是否成功
//////////////////////////////////////////////////////////////////////
void CMatrix::GInvertUV(CMatrix& mtxAP, 
						CMatrix& mtxU, 
						CMatrix& mtxV, 
						double eps /*= 0.000001*/)
{ 
	int i,j,k,l,t,p,q,f;

	// 调用奇异值分解
    SplitUV(mtxU, mtxV, eps);

	int m = rows;
	int n = cols;

	// 初始化广义逆矩阵
	mtxAP.Init(n, m);

	// 计算广义逆矩阵

    j=n;
    if (m<n) 
		j=m;
    j=j-1;
    k=0;
    while ((k<=j)&&(ptr[k*n+k]!=0.0)) 
		k=k+1;

    k=k-1;
    for (i=0; i<=n-1; i++)
	{
		for (j=0; j<=m-1; j++)
		{ 
			t=i*m+j;	
			mtxAP.ptr[t]=0.0;
			for (l=0; l<=k; l++)
			{ 
				f=l*n+i; 
				p=j*m+l; 
				q=l*n+l;
				mtxAP.ptr[t]=mtxAP.ptr[t]+mtxV.ptr[f]*mtxU.ptr[p]/ptr[q];
			}
		}
	}
}

void CMatrix::Init(int r, int c)
{
	if (ptr)
	{
		delete[] ptr;
		ptr = 0;
	}

	rows = r;
	cols = c;
	assert(r*c>0);

	// 分配内存
	ptr = new double[r*c];
	
	assert (ptr != 0);

	// 将各元素值置0
	memset(ptr, 0, sizeof(double) * r * c);
}

//////////////////////////////////////////////////////////////////////
// 一般实矩阵的奇异值分解，分解成功后，原矩阵对角线元素就是矩阵的奇异值
//
// 参数：
// 1. CMatrix& mtxU - 返回分解后的U矩阵
// 2. CMatrix& mtxV - 返回分解后的V矩阵
// 3. double eps - 计算精度，默认值为0.000001
//
// 返回值：BOOL型，求解是否成功
//////////////////////////////////////////////////////////////////////
bool CMatrix::SplitUV(CMatrix& mtxU, CMatrix& mtxV, double eps /*= 0.000001*/)
{ 
	int i,j,k,l,it,ll,kk,ix,iy,mm,nn,iz,m1,ks;
    double d,dd,t,sm,sm1,em1,sk,ek,b,c,shh,fg[2],cs[2];
    double *s,*e,*w;

	int m = rows;
	int n = cols;

	// 初始化U, V矩阵
	mtxU.Init(m, m);
	mtxV.Init(n, n);

	// 临时缓冲区
	int ka = max(m, n) + 1;
    s = new double[ka];
    e = new double[ka];
    w = new double[ka];

	// 指定迭代次数为60
    it=60; 
	k=n;

    if (m-1<n) 
		k=m-1;

    l=m;
    if (n-2<m) 
		l=n-2;
    if (l<0) 
		l=0;

	// 循环迭代计算
    ll=k;
    if (l>k) 
		ll=l;
    if (ll>=1)
    { 
		for (kk=1; kk<=ll; kk++)
        { 
			if (kk<=k)
            { 
				d=0.0;
                for (i=kk; i<=m; i++)
                { 
					ix=(i-1)*n+kk-1; 
					d=d+ptr[ix]*ptr[ix];
				}

                s[kk-1]=sqrt(d);
                if (s[kk-1]!=0.0)
                { 
					ix=(kk-1)*n+kk-1;
                    if (ptr[ix]!=0.0)
                    { 
						s[kk-1]=fabs(s[kk-1]);
                        if (ptr[ix]<0.0) 
							s[kk-1]=-s[kk-1];
                    }
                    
					for (i=kk; i<=m; i++)
                    { 
						iy=(i-1)*n+kk-1;
                        ptr[iy]=ptr[iy]/s[kk-1];
                    }
                    
					ptr[ix]=1.0+ptr[ix];
                }
                
				s[kk-1]=-s[kk-1];
            }
            
			if (n>=kk+1)
            { 
				for (j=kk+1; j<=n; j++)
                { 
					if ((kk<=k)&&(s[kk-1]!=0.0))
                    { 
						d=0.0;
                        for (i=kk; i<=m; i++)
                        { 
							ix=(i-1)*n+kk-1;
                            iy=(i-1)*n+j-1;
                            d=d+ptr[ix]*ptr[iy];
                        }
                        
						d=-d/ptr[(kk-1)*n+kk-1];
                        for (i=kk; i<=m; i++)
                        { 
							ix=(i-1)*n+j-1;
                            iy=(i-1)*n+kk-1;
                            ptr[ix]=ptr[ix]+d*ptr[iy];
                        }
                    }
                    
					e[j-1]=ptr[(kk-1)*n+j-1];
                }
            }
            
			if (kk<=k)
            { 
				for (i=kk; i<=m; i++)
                { 
					ix=(i-1)*m+kk-1; 
					iy=(i-1)*n+kk-1;
                    mtxU.ptr[ix]=ptr[iy];
                }
            }
            
			if (kk<=l)
            { 
				d=0.0;
                for (i=kk+1; i<=n; i++)
					d=d+e[i-1]*e[i-1];
                
				e[kk-1]=sqrt(d);
                if (e[kk-1]!=0.0)
                { 
					if (e[kk]!=0.0)
                    { 
						e[kk-1]=fabs(e[kk-1]);
                        if (e[kk]<0.0) 
							e[kk-1]=-e[kk-1];
                    }

                    for (i=kk+1; i<=n; i++)
                      e[i-1]=e[i-1]/e[kk-1];
                    
					e[kk]=1.0+e[kk];
                }
                
				e[kk-1]=-e[kk-1];
                if ((kk+1<=m)&&(e[kk-1]!=0.0))
                { 
					for (i=kk+1; i<=m; i++) 
						w[i-1]=0.0;
                    
					for (j=kk+1; j<=n; j++)
						for (i=kk+1; i<=m; i++)
							w[i-1]=w[i-1]+e[j-1]*ptr[(i-1)*n+j-1];
                    
					for (j=kk+1; j<=n; j++)
					{
						for (i=kk+1; i<=m; i++)
                        { 
							ix=(i-1)*n+j-1;
							ptr[ix]=ptr[ix]-w[i-1]*e[j-1]/e[kk];
                        }
					}
                }
                
				for (i=kk+1; i<=n; i++)
                  mtxV.ptr[(i-1)*n+kk-1]=e[i-1];
            }
        }
    }
    
	mm=n;
    if (m+1<n) 
		mm=m+1;
    if (k<n) 
		s[k]=ptr[k*n+k];
    if (m<mm) 
		s[mm-1]=0.0;
    if (l+1<mm) 
		e[l]=ptr[l*n+mm-1];

    e[mm-1]=0.0;
    nn=m;
    if (m>n) 
		nn=n;
    if (nn>=k+1)
    { 
		for (j=k+1; j<=nn; j++)
        { 
			for (i=1; i<=m; i++)
				mtxU.ptr[(i-1)*m+j-1]=0.0;
            mtxU.ptr[(j-1)*m+j-1]=1.0;
        }
    }
    
	if (k>=1)
    { 
		for (ll=1; ll<=k; ll++)
        { 
			kk=k-ll+1; 
			iz=(kk-1)*m+kk-1;
            if (s[kk-1]!=0.0)
            { 
				if (nn>=kk+1)
				{
					for (j=kk+1; j<=nn; j++)
					{ 
						d=0.0;
						for (i=kk; i<=m; i++)
						{ 
							ix=(i-1)*m+kk-1;
							iy=(i-1)*m+j-1;
							d=d+mtxU.ptr[ix]*mtxU.ptr[iy]/mtxU.ptr[iz];
						}

						d=-d;
						for (i=kk; i<=m; i++)
						{ 
							ix=(i-1)*m+j-1;
							iy=(i-1)*m+kk-1;
							mtxU.ptr[ix]=mtxU.ptr[ix]+d*mtxU.ptr[iy];
						}
					}
				}
                  
				for (i=kk; i<=m; i++)
				{ 
					ix=(i-1)*m+kk-1; 
					mtxU.ptr[ix]=-mtxU.ptr[ix];
				}

				mtxU.ptr[iz]=1.0+mtxU.ptr[iz];
				if (kk-1>=1)
				{
					for (i=1; i<=kk-1; i++)
						mtxU.ptr[(i-1)*m+kk-1]=0.0;
				}
			}
            else
            { 
				for (i=1; i<=m; i++)
					mtxU.ptr[(i-1)*m+kk-1]=0.0;
                mtxU.ptr[(kk-1)*m+kk-1]=1.0;
            }
		}
    }

    for (ll=1; ll<=n; ll++)
    { 
		kk=n-ll+1; 
		iz=kk*n+kk-1;
        
		if ((kk<=l)&&(e[kk-1]!=0.0))
        { 
			for (j=kk+1; j<=n; j++)
            { 
				d=0.0;
                for (i=kk+1; i<=n; i++)
                { 
					ix=(i-1)*n+kk-1; 
					iy=(i-1)*n+j-1;
                    d=d+mtxV.ptr[ix]*mtxV.ptr[iy]/mtxV.ptr[iz];
                }
                
				d=-d;
                for (i=kk+1; i<=n; i++)
                { 
					ix=(i-1)*n+j-1; 
					iy=(i-1)*n+kk-1;
                    mtxV.ptr[ix]=mtxV.ptr[ix]+d*mtxV.ptr[iy];
                }
            }
        }
        
		for (i=1; i<=n; i++)
			mtxV.ptr[(i-1)*n+kk-1]=0.0;
        
		mtxV.ptr[iz-n]=1.0;
    }
    
	for (i=1; i<=m; i++)
		for (j=1; j<=n; j++)
			ptr[(i-1)*n+j-1]=0.0;
    
	m1=mm; 
	it=60;
    while (true)
    { 
		if (mm==0)
        { 
			ppp(ptr,e,s,mtxV.ptr,m,n);
			delete [] s;
			delete [] e;
			delete [] w;
            return true;
        }
        if (it==0)
        { 
			ppp(ptr,e,s,mtxV.ptr,m,n);
			delete [] s;
			delete [] e;
			delete [] w;
            return false;
        }
        
		kk=mm-1;
		while ((kk!=0)&&(fabs(e[kk-1])!=0.0))
        { 
			d=fabs(s[kk-1])+fabs(s[kk]);
            dd=fabs(e[kk-1]);
            if (dd>eps*d) 
				kk=kk-1;
            else 
				e[kk-1]=0.0;
        }
        
		if (kk==mm-1)
        { 
			kk=kk+1;
            if (s[kk-1]<0.0)
            { 
				s[kk-1]=-s[kk-1];
                for (i=1; i<=n; i++)
                { 
					ix=(i-1)*n+kk-1; 
					mtxV.ptr[ix]=-mtxV.ptr[ix];}
				}
				
				while ((kk!=m1)&&(s[kk-1]<s[kk]))
				{ 
					d=s[kk-1]; 
					s[kk-1]=s[kk]; 
					s[kk]=d;
					if (kk<n)
					{
						for (i=1; i<=n; i++)
						{ 
							ix=(i-1)*n+kk-1; 
							iy=(i-1)*n+kk;
							d=mtxV.ptr[ix]; 
							mtxV.ptr[ix]=mtxV.ptr[iy]; 
							mtxV.ptr[iy]=d;
						}
					}

					if (kk<m)
					{
						for (i=1; i<=m; i++)
						{ 
							ix=(i-1)*m+kk-1; 
							iy=(i-1)*m+kk;
							d=mtxU.ptr[ix]; 
							mtxU.ptr[ix]=mtxU.ptr[iy]; 
							mtxU.ptr[iy]=d;
						}
					}

					kk=kk+1;
            }
            
			it=60;
            mm=mm-1;
        }
        else
        { 
			ks=mm;
            while ((ks>kk)&&(fabs(s[ks-1])!=0.0))
            { 
				d=0.0;
                if (ks!=mm) 
					d=d+fabs(e[ks-1]);
                if (ks!=kk+1) 
					d=d+fabs(e[ks-2]);
                
				dd=fabs(s[ks-1]);
                if (dd>eps*d) 
					ks=ks-1;
                else 
					s[ks-1]=0.0;
            }
            
			if (ks==kk)
            { 
				kk=kk+1;
                d=fabs(s[mm-1]);
                t=fabs(s[mm-2]);
                if (t>d) 
					d=t;
                
				t=fabs(e[mm-2]);
                if (t>d) 
					d=t;
                
				t=fabs(s[kk-1]);
                if (t>d) 
					d=t;
                
				t=fabs(e[kk-1]);
                if (t>d) 
					d=t;
                
				sm=s[mm-1]/d; 
				sm1=s[mm-2]/d;
                em1=e[mm-2]/d;
                sk=s[kk-1]/d; 
				ek=e[kk-1]/d;
                b=((sm1+sm)*(sm1-sm)+em1*em1)/2.0;
                c=sm*em1; 
				c=c*c; 
				shh=0.0;

                if ((b!=0.0)||(c!=0.0))
                { 
					shh=sqrt(b*b+c);
                    if (b<0.0) 
						shh=-shh;

                    shh=c/(b+shh);
                }
                
				fg[0]=(sk+sm)*(sk-sm)-shh;
                fg[1]=sk*ek;
                for (i=kk; i<=mm-1; i++)
                { 
					sss(fg,cs);
                    if (i!=kk) 
						e[i-2]=fg[0];

                    fg[0]=cs[0]*s[i-1]+cs[1]*e[i-1];
                    e[i-1]=cs[0]*e[i-1]-cs[1]*s[i-1];
                    fg[1]=cs[1]*s[i];
                    s[i]=cs[0]*s[i];

                    if ((cs[0]!=1.0)||(cs[1]!=0.0))
					{
						for (j=1; j<=n; j++)
                        { 
							ix=(j-1)*n+i-1;
							iy=(j-1)*n+i;
							d=cs[0]*mtxV.ptr[ix]+cs[1]*mtxV.ptr[iy];
							mtxV.ptr[iy]=-cs[1]*mtxV.ptr[ix]+cs[0]*mtxV.ptr[iy];
							mtxV.ptr[ix]=d;
                        }
					}

                    sss(fg,cs);
                    s[i-1]=fg[0];
                    fg[0]=cs[0]*e[i-1]+cs[1]*s[i];
                    s[i]=-cs[1]*e[i-1]+cs[0]*s[i];
                    fg[1]=cs[1]*e[i];
                    e[i]=cs[0]*e[i];

                    if (i<m)
					{
						if ((cs[0]!=1.0)||(cs[1]!=0.0))
						{
							for (j=1; j<=m; j++)
							{ 
								ix=(j-1)*m+i-1;
								iy=(j-1)*m+i;
								d=cs[0]*mtxU.ptr[ix]+cs[1]*mtxU.ptr[iy];
								mtxU.ptr[iy]=-cs[1]*mtxU.ptr[ix]+cs[0]*mtxU.ptr[iy];
								mtxU.ptr[ix]=d;
							}
						}
					}
                }
                
				e[mm-2]=fg[0];
                it=it-1;
            }
            else
            { 
				if (ks==mm)
                { 
					kk=kk+1;
                    fg[1]=e[mm-2]; 
					e[mm-2]=0.0;
                    for (ll=kk; ll<=mm-1; ll++)
                    { 
						i=mm+kk-ll-1;
                        fg[0]=s[i-1];
                        sss(fg,cs);
                        s[i-1]=fg[0];
                        if (i!=kk)
                        { 
							fg[1]=-cs[1]*e[i-2];
                            e[i-2]=cs[0]*e[i-2];
                        }
                        
						if ((cs[0]!=1.0)||(cs[1]!=0.0))
						{
							for (j=1; j<=n; j++)
                            { 
								ix=(j-1)*n+i-1;
								iy=(j-1)*n+mm-1;
								d=cs[0]*mtxV.ptr[ix]+cs[1]*mtxV.ptr[iy];
								mtxV.ptr[iy]=-cs[1]*mtxV.ptr[ix]+cs[0]*mtxV.ptr[iy];
								mtxV.ptr[ix]=d;
                            }
						}
                    }
                }
                else
                { 
					kk=ks+1;
                    fg[1]=e[kk-2];
                    e[kk-2]=0.0;
                    for (i=kk; i<=mm; i++)
                    { 
						fg[0]=s[i-1];
                        sss(fg,cs);
                        s[i-1]=fg[0];
                        fg[1]=-cs[1]*e[i-1];
                        e[i-1]=cs[0]*e[i-1];
                        if ((cs[0]!=1.0)||(cs[1]!=0.0))
						{
							for (j=1; j<=m; j++)
                            { 
								ix=(j-1)*m+i-1;
								iy=(j-1)*m+kk-2;
								d=cs[0]*mtxU.ptr[ix]+cs[1]*mtxU.ptr[iy];
								mtxU.ptr[iy]=-cs[1]*mtxU.ptr[ix]+cs[0]*mtxU.ptr[iy];
								mtxU.ptr[ix]=d;
                            }
						}
                    }
                }
            }
        }
    }

	delete [] s;
	delete [] e;
	delete [] w;
}
//////////////////////////////////////////////////////////////////////
// 内部函数，由SplitUV函数调用
//////////////////////////////////////////////////////////////////////
void CMatrix::ppp(double a[], double e[], double s[], double v[], int m, int n)
{ 
	int i,j,p,q;
    double d;

    if (m>=n) 
		i=n;
    else 
		i=m;

    for (j=1; j<=i-1; j++)
    { 
		a[(j-1)*n+j-1]=s[j-1];
        a[(j-1)*n+j]=e[j-1];
    }
    
	a[(i-1)*n+i-1]=s[i-1];
    if (m<n) 
		a[(i-1)*n+i]=e[i-1];
    
	for (i=1; i<=n-1; i++)
	{
		for (j=i+1; j<=n; j++)
		{ 
			p=(i-1)*n+j-1; 
			q=(j-1)*n+i-1;
			d=v[p]; 
			v[p]=v[q]; 
			v[q]=d;
		}
	}
}

//////////////////////////////////////////////////////////////////////
// 内部函数，由SplitUV函数调用
//////////////////////////////////////////////////////////////////////
void CMatrix::sss(double fg[2], double cs[2])
{ 
	double r,d;
    
	if ((fabs(fg[0])+fabs(fg[1]))==0.0)
    { 
		cs[0]=1.0; 
		cs[1]=0.0; 
		d=0.0;
	}
    else 
    { 
		d=sqrt(fg[0]*fg[0]+fg[1]*fg[1]);
        if (fabs(fg[0])>fabs(fg[1]))
        { 
			d=fabs(d);
            if (fg[0]<0.0) 
				d=-d;
        }
        if (fabs(fg[1])>=fabs(fg[0]))
        { 
			d=fabs(d);
            if (fg[1]<0.0) 
				d=-d;
        }
        
		cs[0]=fg[0]/d; 
		cs[1]=fg[1]/d;
    }
    
	r=1.0;
    if (fabs(fg[0])>fabs(fg[1])) 
		r=cs[1];
    else if (cs[0]!=0.0) 
		r=1.0/cs[0];

    fg[0]=d; 
	fg[1]=r;
}

//////////////////////////////////////////////////////////////////////
// 求赫申伯格矩阵全部特征值的QR方法
//
// 参数：
// 1. double dblU[] - 一维数组，长度为矩阵的阶数，返回时存放特征值的实部
// 2. double dblV[] - 一维数组，长度为矩阵的阶数，返回时存放特征值的虚部
// 3. int nMaxIt - 迭代次数，默认值为60
// 4. double eps - 计算精度，默认值为0.000001
//
// 返回值：BOOL型，求解是否成功
//////////////////////////////////////////////////////////////////////
void CMatrix::HBergEigenv(double dblU[], double dblV[], int nMaxIt /*= 60*/, double eps /*= 0.000001*/)
{ 
	int m,it,i,j,k,l,ii,jj,kk,ll;
    double b,c,w,g,xy,p,q,r,x,s,e,f,z,y;
    
	int n = cols;

	it=0; 
	m=n;
    while (m!=0)
    { 
		l=m-1;
        while ((l>0)&&(fabs(ptr[l*n+l-1]) > 
				eps*(fabs(ptr[(l-1)*n+l-1])+fabs(ptr[l*n+l])))) 
		  l=l-1;

        ii=(m-1)*n+m-1; 
		jj=(m-1)*n+m-2;
        kk=(m-2)*n+m-1; 
		ll=(m-2)*n+m-2;
        if (l==m-1)
        { 
			dblU[m-1]=ptr[(m-1)*n+m-1]; 
			dblV[m-1]=0.0;
            m=m-1; 
			it=0;
        }
        else if (l==m-2)
        { 
			b=-(ptr[ii]+ptr[ll]);
            c=ptr[ii]*ptr[ll]-ptr[jj]*ptr[kk];
            w=b*b-4.0*c;
            y=sqrt(fabs(w));
            if (w>0.0)
            { 
				xy=1.0;
                if (b<0.0) 
					xy=-1.0;
                dblU[m-1]=(-b-xy*y)/2.0;
                dblU[m-2]=c/dblU[m-1];
                dblV[m-1]=0.0; dblV[m-2]=0.0;
            }
            else
            { 
				dblU[m-1]=-b/2.0; 
				dblU[m-2]=dblU[m-1];
                dblV[m-1]=y/2.0; 
				dblV[m-2]=-dblV[m-1];
            }
            
			m=m-2; 
			it=0;
        }
        else
        { 
			assert (it<nMaxIt);

            it=it+1;
            for (j=l+2; j<=m-1; j++)
				ptr[j*n+j-2]=0.0;
            for (j=l+3; j<=m-1; j++)
				ptr[j*n+j-3]=0.0;
            for (k=l; k<=m-2; k++)
            { 
				if (k!=l)
                { 
					p=ptr[k*n+k-1]; 
					q=ptr[(k+1)*n+k-1];
                    r=0.0;
                    if (k!=m-2) 
						r=ptr[(k+2)*n+k-1];
                }
                else
                { 
					x=ptr[ii]+ptr[ll];
                    y=ptr[ll]*ptr[ii]-ptr[kk]*ptr[jj];
                    ii=l*n+l; 
					jj=l*n+l+1;
                    kk=(l+1)*n+l; 
					ll=(l+1)*n+l+1;
                    p=ptr[ii]*(ptr[ii]-x)+ptr[jj]*ptr[kk]+y;
                    q=ptr[kk]*(ptr[ii]+ptr[ll]-x);
                    r=ptr[kk]*ptr[(l+2)*n+l+1];
                }
                
				if ((fabs(p)+fabs(q)+fabs(r))!=0.0)
                { 
					xy=1.0;
                    if (p<0.0) 
						xy=-1.0;
                    s=xy*sqrt(p*p+q*q+r*r);
                    if (k!=l) 
						ptr[k*n+k-1]=-s;
                    e=-q/s; 
					f=-r/s; 
					x=-p/s;
                    y=-x-f*r/(p+s);
                    g=e*r/(p+s);
                    z=-x-e*q/(p+s);
                    for (j=k; j<=m-1; j++)
                    { 
						ii=k*n+j; 
						jj=(k+1)*n+j;
                        p=x*ptr[ii]+e*ptr[jj];
                        q=e*ptr[ii]+y*ptr[jj];
                        r=f*ptr[ii]+g*ptr[jj];
                        if (k!=m-2)
                        { 
							kk=(k+2)*n+j;
                            p=p+f*ptr[kk];
                            q=q+g*ptr[kk];
                            r=r+z*ptr[kk]; 
							ptr[kk]=r;
                        }
                        
						ptr[jj]=q; ptr[ii]=p;
                    }
                    
					j=k+3;
                    if (j>=m-1) 
						j=m-1;
                    
					for (i=l; i<=j; i++)
                    { 
						ii=i*n+k; 
						jj=i*n+k+1;
                        p=x*ptr[ii]+e*ptr[jj];
                        q=e*ptr[ii]+y*ptr[jj];
                        r=f*ptr[ii]+g*ptr[jj];
                        if (k!=m-2)
                        { 
							kk=i*n+k+2;
                            p=p+f*ptr[kk];
                            q=q+g*ptr[kk];
                            r=r+z*ptr[kk]; 
							ptr[kk]=r;
                        }
                        
						ptr[jj]=q; 
						ptr[ii]=p;
                    }
                }
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////
// 实矩阵求逆的全选主元高斯－约当法
//
// 参数：无
//
// 返回值：BOOL型，求逆是否成功
//////////////////////////////////////////////////////////////////////
void CMatrix::InvertGaussJordan()
{
	int *pnRow, *pnCol,i,j,k,l,u,v;
    double d = 0, p = 0;

	// 分配内存
    pnRow = new int[cols];
    pnCol = new int[cols];
	assert (pnRow != 0 && pnCol != 0);

	// 消元
    for (k=0; k<=cols-1; k++)
    { 
		d=0.0;
        for (i=k; i<=cols-1; i++)
		{
			for (j=k; j<=cols-1; j++)
			{ 
				l=i*cols+j; p=fabs(ptr[l]);
				if (p>d) 
				{ 
					d=p; 
					pnRow[k]=i; 
					pnCol[k]=j;
				}
			}
		}
        
		// 失败
        if (d == 0.0)
        { 
			delete[] pnRow;
			delete[] pnCol;
        }
		assert(d != 0.0);

        if (pnRow[k] != k)
		{
			for (j=0; j<=cols-1; j++)
			{ 
				u=k*cols+j; 
				v=pnRow[k]*cols+j;
				p=ptr[u]; 
				ptr[u]=ptr[v]; 
				ptr[v]=p;
			}
		}
        
		if (pnCol[k] != k)
		{
			for (i=0; i<=cols-1; i++)
            { 
				u=i*cols+k; 
				v=i*cols+pnCol[k];
				p=ptr[u]; 
				ptr[u]=ptr[v]; 
				ptr[v]=p;
            }
		}

        l=k*cols+k;
        ptr[l]=1.0/ptr[l];
        for (j=0; j<=cols-1; j++)
		{
			if (j != k)
            { 
				u=k*cols+j; 
				ptr[u]=ptr[u]*ptr[l];
			}
		}

        for (i=0; i<=cols-1; i++)
		{
			if (i!=k)
			{
				for (j=0; j<=cols-1; j++)
				{
					if (j!=k)
					{ 
						u=i*cols+j;
						ptr[u]=ptr[u]-ptr[i*cols+k]*ptr[k*cols+j];
					}
                }
			}
		}

        for (i=0; i<=cols-1; i++)
		{
			if (i!=k)
            { 
				u=i*cols+k; 
				ptr[u]=-ptr[u]*ptr[l];
			}
		}
    }

    // 调整恢复行列次序
    for (k=cols-1; k>=0; k--)
    { 
		if (pnCol[k]!=k)
		{
			for (j=0; j<=cols-1; j++)
            { 
				u=k*cols+j; 
				v=pnCol[k]*cols+j;
				p=ptr[u]; 
				ptr[u]=ptr[v]; 
				ptr[v]=p;
            }
		}

        if (pnRow[k]!=k)
		{
			for (i=0; i<=cols-1; i++)
            { 
				u=i*cols+k; 
				v=i*cols+pnRow[k];
				p=ptr[u]; 
				ptr[u]=ptr[v]; 
				ptr[v]=p;
            }
		}
    }

	// 清理内存
	delete[] pnRow;
	delete[] pnCol;
}

//////////////////////////////////////////////////////////////////////
// 复矩阵求逆的全选主元高斯－约当法
//
// 参数：
// 1. CMatrix& mtxImag - 复矩阵的虚部矩阵，当前矩阵为复矩阵的实部
//
// 返回值：BOOL型，求逆是否成功
//////////////////////////////////////////////////////////////////////
void CMatrix::InvertGaussJordan(CMatrix& mtxImag)
{
	int *pnRow,*pnCol,i,j,k,l,u,v,w;
    double p,q,s,t,d,b;

	// 分配内存
    pnRow = new int[cols];
    pnCol = new int[cols];
	assert (pnRow != 0 && pnCol != 0);

	// 消元
    for (k=0; k<=cols-1; k++)
    { 
		d=0.0;
        for (i=k; i<=cols-1; i++)
		{
			for (j=k; j<=cols-1; j++)
			{ 
				u=i*cols+j;
				p=ptr[u]*ptr[u]+mtxImag.ptr[u]*mtxImag.ptr[u];
				if (p>d) 
				{ 
					d=p; 
					pnRow[k]=i; 
					pnCol[k]=j;
				}
			}
		}

		// 失败
        if (d == 0.0)
        { 
			delete[] pnRow;
			delete[] pnCol;
        }
		assert(d != 0.0);

        if (pnRow[k]!=k)
		{
			for (j=0; j<=cols-1; j++)
            { 
				u=k*cols+j; 
				v=pnRow[k]*cols+j;
				t=ptr[u]; 
				ptr[u]=ptr[v]; 
				ptr[v]=t;
				t=mtxImag.ptr[u]; 
				mtxImag.ptr[u]=mtxImag.ptr[v]; 
				mtxImag.ptr[v]=t;
            }
		}

        if (pnCol[k]!=k)
		{
			for (i=0; i<=cols-1; i++)
            { 
				u=i*cols+k; 
				v=i*cols+pnCol[k];
				t=ptr[u]; 
				ptr[u]=ptr[v]; 
				ptr[v]=t;
				t=mtxImag.ptr[u]; 
				mtxImag.ptr[u]=mtxImag.ptr[v]; 
				mtxImag.ptr[v]=t;
            }
		}

        l=k*cols+k;
        ptr[l]=ptr[l]/d; mtxImag.ptr[l]=-mtxImag.ptr[l]/d;
        for (j=0; j<=cols-1; j++)
		{
			if (j!=k)
            { 
				u=k*cols+j;
				p=ptr[u]*ptr[l]; 
				q=mtxImag.ptr[u]*mtxImag.ptr[l];
				s=(ptr[u]+mtxImag.ptr[u])*(ptr[l]+mtxImag.ptr[l]);
				ptr[u]=p-q; 
				mtxImag.ptr[u]=s-p-q;
            }
		}

        for (i=0; i<=cols-1; i++)
		{
			if (i!=k)
            { 
				v=i*cols+k;
				for (j=0; j<=cols-1; j++)
				{
					if (j!=k)
					{ 
						u=k*cols+j;  
						w=i*cols+j;
						p=ptr[u]*ptr[v]; 
						q=mtxImag.ptr[u]*mtxImag.ptr[v];
						s=(ptr[u]+mtxImag.ptr[u])*(ptr[v]+mtxImag.ptr[v]);
						t=p-q; 
						b=s-p-q;
						ptr[w]=ptr[w]-t;
						mtxImag.ptr[w]=mtxImag.ptr[w]-b;
					}
				}
            }
		}

        for (i=0; i<=cols-1; i++)
		{
			if (i!=k)
            { 
				u=i*cols+k;
				p=ptr[u]*ptr[l]; 
				q=mtxImag.ptr[u]*mtxImag.ptr[l];
				s=(ptr[u]+mtxImag.ptr[u])*(ptr[l]+mtxImag.ptr[l]);
				ptr[u]=q-p; 
				mtxImag.ptr[u]=p+q-s;
            }
		}
    }

    // 调整恢复行列次序
    for (k=cols-1; k>=0; k--)
    { 
		if (pnCol[k]!=k)
		{
			for (j=0; j<=cols-1; j++)
            { 
				u=k*cols+j; 
				v=pnCol[k]*cols+j;
				t=ptr[u]; 
				ptr[u]=ptr[v]; 
				ptr[v]=t;
				t=mtxImag.ptr[u]; 
				mtxImag.ptr[u]=mtxImag.ptr[v]; 
				mtxImag.ptr[v]=t;
            }
		}

        if (pnRow[k]!=k)
		{
			for (i=0; i<=cols-1; i++)
            { 
				u=i*cols+k; 
				v=i*cols+pnRow[k];
				t=ptr[u]; 
				ptr[u]=ptr[v]; 
				ptr[v]=t;
				t=mtxImag.ptr[u]; 
				mtxImag.ptr[u]=mtxImag.ptr[v]; 
				mtxImag.ptr[v]=t;
            }
		}
    }

	// 清理内存
	delete[] pnRow;
	delete[] pnCol;
}

//////////////////////////////////////////////////////////////////////
// 对称正定矩阵的求逆
//
// 参数：无
//
// 返回值：BOOL型，求逆是否成功
//////////////////////////////////////////////////////////////////////
void CMatrix::InvertSsgj()
{ 
	int i, j ,k, m;
    double w, g, *pTmp;

	// 临时内存
    pTmp = new double[cols];
	assert(pTmp != 0);

	// 逐列处理
    for (k=0; k<=cols-1; k++)
    { 
		w=ptr[0];
        if (w == 0.0)
        { 
			delete[] pTmp;
		}
		assert(w != 0.0);

        m=cols-k-1;
        for (i=1; i<=cols-1; i++)
        { 
			g=ptr[i*cols]; 
			pTmp[i]=g/w;
            if (i<=m) 
				pTmp[i]=-pTmp[i];
            for (j=1; j<=i; j++)
              ptr[(i-1)*cols+j-1]=ptr[i*cols+j]+g*pTmp[j];
        }

        ptr[cols*cols-1]=1.0/w;
        for (i=1; i<=cols-1; i++)
			ptr[(cols-1)*cols+i-1]=pTmp[i];
    }

	// 行列调整
    for (i=0; i<=cols-2; i++)
		for (j=i+1; j<=cols-1; j++)
			ptr[i*cols+j]=ptr[j*cols+i];

	// 临时内存清理
	delete[] pTmp;
}

//////////////////////////////////////////////////////////////////////
// 托伯利兹矩阵求逆的埃兰特方法
//
// 参数：无
//
// 返回值：BOOL型，求逆是否成功
//////////////////////////////////////////////////////////////////////
void CMatrix::InvertTrench()
{ 
	int i,j,k;
    double a,s,*t,*tt,*c,*r,*p;

	// 上三角元素
	t = new double[cols];
	// 下三角元素
	tt = new double[cols];

	assert(t != 0 && tt != 0);

	// 上、下三角元素赋值
	for (i=0; i<cols; ++i)
	{
		t[i] = ptr[0*cols+i];
	    tt[i] = ptr[i*cols+0];
	}

	// 非Toeplitz矩阵，返回
    if (t[0] == 0.0)
    { 
		delete[] t;
		delete[] tt;
    }
	assert(t[0] != 0.0);

	// 临时缓冲区
	c = new double[cols];
	r = new double[cols];
	p = new double[cols];

	assert(c != 0 && r != 0 && p != 0);
	
    a=t[0]; 
	c[0]=tt[1]/t[0]; 
	r[0]=t[1]/t[0];

    for (k=0; k<=cols-3; k++)
    { 
		s=0.0;
        for (j=1; j<=k+1; j++)
			s=s+c[k+1-j]*tt[j];

        s=(s-tt[k+2])/a;
		for (i=0; i<=k; i++)
			p[i]=c[i]+s*r[k-i];

        c[k+1]=-s;
        s=0.0;
        for (j=1; j<=k+1; j++)
          s=s+r[k+1-j]*t[j];
        
		s=(s-t[k+2])/a;
        for (i=0; i<=k; i++)
        { 
			r[i]=r[i]+s*c[k-i];
            c[k-i]=p[k-i];
        }

        r[k+1]=-s;
		a=0.0;
        for (j=1; j<=k+2; j++)
          a=a+t[j]*c[j-1];

        a=t[0]-a;

		// 求解失败
        if (a == 0.0)
		{ 
			delete[] t;
			delete[] tt;
			delete[] c;
			delete[] r;
			delete[] p;
		}
		assert(a != 0.0);
    }

    ptr[0]=1.0/a;
    for (i=0; i<=cols-2; i++)
    { 
		k=i+1; 
		j=(i+1)*cols;
        ptr[k]=-r[i]/a; 
		ptr[j]=-c[i]/a;
    }

   for (i=0; i<=cols-2; i++)
	{
		for (j=0; j<=cols-2; j++)
		{ 
			k=(i+1)*cols+j+1;
			ptr[k]=ptr[i*cols+j]-c[i]*ptr[j+1];
			ptr[k]=ptr[k]+c[cols-j-2]*ptr[cols-i-1];
		}
	}

    // 临时内存清理
	delete[] t;
	delete[] tt;
	delete[] c;
	delete[] r;
	delete[] p;
}
              
//////////////////////////////////////////////////////////////////////
// 求实对称矩阵特征值与特征向量的雅可比法
//
// 参数：
// 1. double dblEigenValue[] - 一维数组，长度为矩阵的阶数，返回时存放特征值
// 2. CMatrix& mtxEigenVector - 返回时存放特征向量矩阵，其中第i列为与
//    数组dblEigenValue中第j个特征值对应的特征向量
// 3. int nMaxIt - 迭代次数，默认值为60
// 4. double eps - 计算精度，默认值为0.000001
//
// 返回值：BOOL型，求解是否成功
//////////////////////////////////////////////////////////////////////
void CMatrix::JacobiEigenv(double dblEigenValue[], 
						   CMatrix& mtxEigenVector, 
						   int nMaxIt /*= 60*/, 
						   double eps /*= 0.000001*/)
{ 
	int i,j,p,q,u,w,t,s,l;
    double fm,cn,sn,omega,x,y,d;
    
	mtxEigenVector.Init(cols, cols);

	l=1;
    for (i=0; i<=cols-1; i++)
    { 
		mtxEigenVector.ptr[i*cols+i]=1.0;
        for (j=0; j<=cols-1; j++)
			if (i!=j) 
				mtxEigenVector.ptr[i*cols+j]=0.0;
    }
    
	while (true)
    { 
		fm=0.0;
        for (i=1; i<=cols-1; i++)
		{
			for (j=0; j<=i-1; j++)
			{ 
				d=fabs(ptr[i*cols+j]);
				if ((i!=j)&&(d>fm))
				{ 
					fm=d; 
					p=i; 
					q=j;
				}
			}
		}

        if (fm<eps)
		{
			for (i=0; i<cols; ++i)
				dblEigenValue[i] = ptr[i*cols+i];
			return;
		}

        assert (l<=nMaxIt);

		l=l+1;
        u=p*cols+q; 
		w=p*cols+p; 
		t=q*cols+p; 
		s=q*cols+q;
        x=-ptr[u]; 
		y=(ptr[s]-ptr[w])/2.0;
        omega=x/sqrt(x*x+y*y);

        if (y<0.0) 
			omega=-omega;

        sn=1.0+sqrt(1.0-omega*omega);
        sn=omega/sqrt(2.0*sn);
        cn=sqrt(1.0-sn*sn);
        fm=ptr[w];
        ptr[w]=fm*cn*cn+ptr[s]*sn*sn+ptr[u]*omega;
        ptr[s]=fm*sn*sn+ptr[s]*cn*cn-ptr[u]*omega;
        ptr[u]=0.0; 
		ptr[t]=0.0;
        for (j=0; j<=cols-1; j++)
		{
			if ((j!=p)&&(j!=q))
			{ 
				u=p*cols+j; w=q*cols+j;
				fm=ptr[u];
				ptr[u]=fm*cn+ptr[w]*sn;
				ptr[w]=-fm*sn+ptr[w]*cn;
			}
		}

        for (i=0; i<=cols-1; i++)
		{
			if ((i!=p)&&(i!=q))
            { 
				u=i*cols+p; 
				w=i*cols+q;
				fm=ptr[u];
				ptr[u]=fm*cn+ptr[w]*sn;
				ptr[w]=-fm*sn+ptr[w]*cn;
            }
		}

        for (i=0; i<=cols-1; i++)
        { 
			u=i*cols+p; 
			w=i*cols+q;
            fm=mtxEigenVector.ptr[u];
            mtxEigenVector.ptr[u]=fm*cn+mtxEigenVector.ptr[w]*sn;
            mtxEigenVector.ptr[w]=-fm*sn+mtxEigenVector.ptr[w]*cn;
        }
    }
    
	for (i=0; i<cols; ++i)
		dblEigenValue[i] = ptr[i*cols+i];
}

//////////////////////////////////////////////////////////////////////
// 求实对称矩阵特征值与特征向量的雅可比过关法
//
// 参数：
// 1. double dblEigenValue[] - 一维数组，长度为矩阵的阶数，返回时存放特征值
// 2. CMatrix& mtxEigenVector - 返回时存放特征向量矩阵，其中第i列为与
//    数组dblEigenValue中第j个特征值对应的特征向量
// 3. double eps - 计算精度，默认值为0.000001
//
// 返回值：BOOL型，求解是否成功
//////////////////////////////////////////////////////////////////////
void CMatrix::JacobiEigenv2(double dblEigenValue[], 
							CMatrix& mtxEigenVector, 
							double eps /*= 0.000001*/)
{ 
	int i,j,p,q,u,w,t,s;
    double ff,fm,cn,sn,omega,x,y,d;
    
	mtxEigenVector.Init(cols, cols);

	for (i=0; i<=cols-1; i++)
    { 
		mtxEigenVector.ptr[i*cols+i]=1.0;
        for (j=0; j<=cols-1; j++)
			if (i!=j) 
				mtxEigenVector.ptr[i*cols+j]=0.0;
    }
    
	ff=0.0;
    for (i=1; i<=cols-1; i++)
	{
		for (j=0; j<=i-1; j++)
		{ 
			d=ptr[i*cols+j]; 
			ff=ff+d*d; 
		}
	}

    ff=sqrt(2.0*ff);

Loop_0:
    
	ff=ff/(1.0*cols);

Loop_1:

    for (i=1; i<=cols-1; i++)
	{
		for (j=0; j<=i-1; j++)
        { 
			d=fabs(ptr[i*cols+j]);
            if (d>ff)
            { 
				p=i; 
				q=j;
                goto Loop_2;
            }
        }
	}
        
	if (ff<eps) 
	{
		for (i=0; i<cols; ++i)
			dblEigenValue[i] = ptr[i*cols+i];
		return;
	}
    
	goto Loop_0;

Loop_2: 
		
	u=p*cols+q; 
	w=p*cols+p; 
	t=q*cols+p; 
	s=q*cols+q;
    x=-ptr[u]; 
	y=(ptr[s]-ptr[w])/2.0;
    omega=x/sqrt(x*x+y*y);
    if (y<0.0) 
		omega=-omega;
    
	sn=1.0+sqrt(1.0-omega*omega);
    sn=omega/sqrt(2.0*sn);
    cn=sqrt(1.0-sn*sn);
    fm=ptr[w];
    ptr[w]=fm*cn*cn+ptr[s]*sn*sn+ptr[u]*omega;
    ptr[s]=fm*sn*sn+ptr[s]*cn*cn-ptr[u]*omega;
    ptr[u]=0.0; ptr[t]=0.0;
    
	for (j=0; j<=cols-1; j++)
	{
		if ((j!=p)&&(j!=q))
		{ 
			u=p*cols+j; 
			w=q*cols+j;
			fm=ptr[u];
			ptr[u]=fm*cn+ptr[w]*sn;
			ptr[w]=-fm*sn+ptr[w]*cn;
		}
	}

    for (i=0; i<=cols-1; i++)
    {
		if ((i!=p)&&(i!=q))
        { 
			u=i*cols+p; 
			w=i*cols+q;
			fm=ptr[u];
			ptr[u]=fm*cn+ptr[w]*sn;
			ptr[w]=-fm*sn+ptr[w]*cn;
        }
	}
    
	for (i=0; i<=cols-1; i++)
    { 
		u=i*cols+p; 
		w=i*cols+q;
        fm=mtxEigenVector.ptr[u];
        mtxEigenVector.ptr[u]=fm*cn+mtxEigenVector.ptr[w]*sn;
        mtxEigenVector.ptr[w]=-fm*sn+mtxEigenVector.ptr[w]*cn;
	}

	goto Loop_1;
}

//////////////////////////////////////////////////////////////////////
// 约化一般实矩阵为赫申伯格矩阵的初等相似变换法
//
// 参数：无
//
// 返回值：无
//////////////////////////////////////////////////////////////////////
void CMatrix::MakeHberg()
{ 
	int i,j,k,u,v;
    double d,t;

    for (k=1; k<=cols-2; k++)
    { 
		d=0.0;
        for (j=k; j<=cols-1; j++)
        { 
			u=j*cols+k-1; 
			t=ptr[u];
            if (fabs(t)>fabs(d))
            { 
				d=t; 
				i=j;
			}
        }
        
		if (d != 0.0)
        { 
			if (i!=k)
            { 
				for (j=k-1; j<=cols-1; j++)
                { 
					u=i*cols+j; 
					v=k*cols+j;
                    t=ptr[u]; 
					ptr[u]=ptr[v]; 
					ptr[v]=t;
                }
                
				for (j=0; j<=cols-1; j++)
                { 
					u=j*cols+i; 
					v=j*cols+k;
                    t=ptr[u]; 
					ptr[u]=ptr[v]; 
					ptr[v]=t;
                }
            }
            
			for (i=k+1; i<=cols-1; i++)
            { 
				u=i*cols+k-1; 
				t=ptr[u]/d; 
				ptr[u]=0.0;
                for (j=k; j<=cols-1; j++)
                { 
					v=i*cols+j;
                    ptr[v]=ptr[v]-t*ptr[k*cols+j];
                }
                
				for (j=0; j<=cols-1; j++)
                { 
					v=j*cols+k;
                    ptr[v]=ptr[v]+t*ptr[j*cols+i];
                }
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////
// 约化对称矩阵为对称三对角阵的豪斯荷尔德变换法
//
// 参数：
// 1. CMatrix& mtxQ - 返回豪斯荷尔德变换的乘积矩阵Q
// 2. CMatrix& mtxT - 返回求得的对称三对角阵
// 3. double dblB[] - 一维数组，长度为矩阵的阶数，返回对称三对角阵的主对角线元素
// 4. double dblC[] - 一维数组，长度为矩阵的阶数，前n-1个元素返回对称三对角阵的次对角线元素
//
// 返回值：BOOL型，求解是否成功
//////////////////////////////////////////////////////////////////////
void CMatrix::MakeSymTri(CMatrix& mtxQ, 
						 CMatrix& mtxT, 
						 double dblB[], double dblC[])
{ 
	int i,j,k,u;
    double h,f,g,h2;
    
	// 初始化矩阵Q和T
	mtxQ.Init(cols, cols);
	mtxT.Init(cols, cols);

	assert (dblB != 0 && dblC != 0);

	for (i=0; i<=cols-1; i++)
	{
		for (j=0; j<=cols-1; j++)
		{ 
			u=i*cols+j; 
			mtxQ.ptr[u]=ptr[u];
		}
	}

    for (i=cols-1; i>=1; i--)
    { 
		h=0.0;
        if (i>1)
		{
			for (k=0; k<=i-1; k++)
            { 
				u=i*cols+k; 
				h=h+mtxQ.ptr[u]*mtxQ.ptr[u];
			}
		}

        if (h == 0.0)
        { 
			dblC[i]=0.0;
            if (i==1) 
				dblC[i]=mtxQ.ptr[i*cols+i-1];
            dblB[i]=0.0;
        }
        else
        { 
			dblC[i]=sqrt(h);
            u=i*cols+i-1;
            if (mtxQ.ptr[u]>0.0) 
				dblC[i]=-dblC[i];

            h=h-mtxQ.ptr[u]*dblC[i];
            mtxQ.ptr[u]=mtxQ.ptr[u]-dblC[i];
            f=0.0;
            for (j=0; j<=i-1; j++)
            { 
				mtxQ.ptr[j*cols+i]=mtxQ.ptr[i*cols+j]/h;
                g=0.0;
                for (k=0; k<=j; k++)
					g=g+mtxQ.ptr[j*cols+k]*mtxQ.ptr[i*cols+k];

				if (j+1<=i-1)
					for (k=j+1; k<=i-1; k++)
						g=g+mtxQ.ptr[k*cols+j]*mtxQ.ptr[i*cols+k];

                dblC[j]=g/h;
                f=f+g*mtxQ.ptr[j*cols+i];
            }
            
			h2=f/(h+h);
            for (j=0; j<=i-1; j++)
            { 
				f=mtxQ.ptr[i*cols+j];
                g=dblC[j]-h2*f;
                dblC[j]=g;
                for (k=0; k<=j; k++)
                { 
					u=j*cols+k;
                    mtxQ.ptr[u]=mtxQ.ptr[u]-f*dblC[k]-g*mtxQ.ptr[i*cols+k];
                }
            }
            
			dblB[i]=h;
        }
    }
    
	for (i=0; i<=cols-2; i++) 
		dblC[i]=dblC[i+1];
    
	dblC[cols-1]=0.0;
    dblB[0]=0.0;
    for (i=0; i<=cols-1; i++)
    { 
		if ((dblB[i]!=0.0)&&(i-1>=0))
		{
			for (j=0; j<=i-1; j++)
            { 
				g=0.0;
				for (k=0; k<=i-1; k++)
					g=g+mtxQ.ptr[i*cols+k]*mtxQ.ptr[k*cols+j];

				for (k=0; k<=i-1; k++)
                { 
					u=k*cols+j;
					mtxQ.ptr[u]=mtxQ.ptr[u]-g*mtxQ.ptr[k*cols+i];
                }
            }
		}

        u=i*cols+i;
        dblB[i]=mtxQ.ptr[u]; mtxQ.ptr[u]=1.0;
        if (i-1>=0)
		{
			for (j=0; j<=i-1; j++)
            { 
				mtxQ.ptr[i*cols+j]=0.0; 
				mtxQ.ptr[j*cols+i]=0.0;
			}
		}
    }

    // 构造对称三对角矩阵
    for (i=0; i<cols; ++i)
	{
	    for (j=0; j<cols; ++j)
		{
            mtxT(i, j) = 0;
            k = i - j;
            if (k == 0) 
	            mtxT(i, j) = dblB[j];
			else if (k == 1)
	            mtxT(i, j) = dblC[j];
			else if (k == -1)
	            mtxT(i, j) = dblC[i];
        }
    }
}

//////////////////////////////////////////////////////////////////////
// 求矩阵秩的全选主元高斯消去法
//
// 参数：无
//
// 返回值：int型，矩阵的秩
//////////////////////////////////////////////////////////////////////
int CMatrix::RankGauss()
{ 
	int i,j,k,nn,is,js,l,ll,u,v;
    double q,d;
    
	// 秩小于等于行列数
	nn = rows;
    if (rows >= cols) 
		nn = cols;

    k=0;

	// 消元求解
    for (l=0; l<=nn-1; l++)
    { 
		q=0.0;
        for (i=l; i<=rows-1; i++)
		{
			for (j=l; j<=cols-1; j++)
			{ 
				ll=i*cols+j; 
				d=fabs(ptr[ll]);
				if (d>q) 
				{ 
					q=d; 
					is=i; 
					js=j;
				}
			}
		}

        if (q == 0.0) 
			return(k);

        k=k+1;
        if (is!=l)
        { 
			for (j=l; j<=cols-1; j++)
            { 
				u=l*cols+j; 
				v=is*cols+j;
                d=ptr[u]; 
				ptr[u]=ptr[v]; 
				ptr[v]=d;
            }
        }
        if (js!=l)
        { 
			for (i=l; i<=rows-1; i++)
            { 
				u=i*cols+js; 
				v=i*cols+l;
                d=ptr[u]; 
				ptr[u]=ptr[v]; 
				ptr[v]=d;
            }
        }
        
		ll=l*cols+l;
        for (i=l+1; i<=cols-1; i++)
        { 
			d=ptr[i*cols+l]/ptr[ll];
            for (j=l+1; j<=cols-1; j++)
            { 
				u=i*cols+j;
                ptr[u]=ptr[u]-d*ptr[l*cols+j];
            }
        }
    }
    
	return(k);
}

//////////////////////////////////////////////////////////////////////
// 矩阵的三角分解，分解成功后，原矩阵将成为Q矩阵
//
// 参数：
// 1. CMatrix& mtxL - 返回分解后的L矩阵
// 2. CMatrix& mtxU - 返回分解后的U矩阵
//
// 返回值：BOOL型，求解是否成功
//////////////////////////////////////////////////////////////////////
void CMatrix::SplitLU(CMatrix& mtxL, CMatrix& mtxU)
{ 
	int i,j,k,w,v,ll;
    
	// 初始化结果矩阵
	mtxL.Init(cols, cols);
	mtxU.Init(cols, cols);

	for (k=0; k<=cols-2; k++)
    { 
		ll=k*cols+k;
		assert(ptr[ll] != 0.0);

        for (i=k+1; i<=cols-1; i++)
		{ 
			w=i*cols+k; 
			ptr[w]=ptr[w]/ptr[ll];
		}

        for (i=k+1; i<=cols-1; i++)
        { 
			w=i*cols+k;
            for (j=k+1; j<=cols-1; j++)
            { 
				v=i*cols+j;
                ptr[v]=ptr[v]-ptr[w]*ptr[k*cols+j];
            }
        }
    }
    
	for (i=0; i<=cols-1; i++)
    {
		for (j=0; j<i; j++)
        { 
			w=i*cols+j; 
			mtxL.ptr[w]=ptr[w]; 
			mtxU.ptr[w]=0.0;
		}

        w=i*cols+i;
        mtxL.ptr[w]=1.0; 
		mtxU.ptr[w]=ptr[w];
        
		for (j=i+1; j<=cols-1; j++)
        { 
			w=i*cols+j; 
			mtxL.ptr[w]=0.0; 
			mtxU.ptr[w]=ptr[w];
		}
    }
}

//////////////////////////////////////////////////////////////////////
// 一般实矩阵的QR分解，分解成功后，原矩阵将成为R矩阵
//
// 参数：
// 1. CMatrix& mtxQ - 返回分解后的Q矩阵
//
// 返回值：BOOL型，求解是否成功
//////////////////////////////////////////////////////////////////////
void CMatrix::SplitQR(CMatrix& mtxQ)
{ 
	int i,j,k,l,nn,p,jj;
    double u,alpha,w,t;
    
	assert (rows >= cols);

	// 初始化Q矩阵
	mtxQ.Init(rows, rows);

	// 对角线元素单位化
    for (i=0; i<=rows-1; i++)
	{
		for (j=0; j<=rows-1; j++)
		{ 
			l=i*rows+j; 
			mtxQ.ptr[l]=0.0;
			if (i==j) 
				mtxQ.ptr[l]=1.0;
		}
	}

	// 开始分解

    nn=cols;
    if (rows == cols) 
		nn=rows-1;

    for (k=0; k<=nn-1; k++)
    { 
		u=0.0; 
		l=k*cols+k;
        for (i=k; i<=rows-1; i++)
        { 
			w=fabs(ptr[i*cols+k]);
            if (w>u) 
				u=w;
        }
        
		alpha=0.0;
        for (i=k; i<=rows-1; i++)
        { 
			t=ptr[i*cols+k]/u; 
			alpha=alpha+t*t;
		}

        if (ptr[l]>0.0) 
			u=-u;

        alpha=u*sqrt(alpha);
        assert (alpha != 0.0);

        u=sqrt(2.0*alpha*(alpha-ptr[l]));
        if ((u+1.0)!=1.0)
        { 
			ptr[l]=(ptr[l]-alpha)/u;
            for (i=k+1; i<=rows-1; i++)
            { 
				p=i*cols+k; 
				ptr[p]=ptr[p]/u;
			}
            
			for (j=0; j<=rows-1; j++)
            { 
				t=0.0;
                for (jj=k; jj<=rows-1; jj++)
					t=t+ptr[jj*cols+k]*mtxQ.ptr[jj*rows+j];

                for (i=k; i<=rows-1; i++)
                { 
					p=i*rows+j; 
					mtxQ.ptr[p]=mtxQ.ptr[p]-2.0*t*ptr[i*cols+k];
				}
            }
            
			for (j=k+1; j<=cols-1; j++)
            { 
				t=0.0;
                
				for (jj=k; jj<=rows-1; jj++)
					t=t+ptr[jj*cols+k]*ptr[jj*cols+j];
                
				for (i=k; i<=rows-1; i++)
                { 
					p=i*cols+j; 
					ptr[p]=ptr[p]-2.0*t*ptr[i*cols+k];
				}
            }
            
			ptr[l]=alpha;
            for (i=k+1; i<=rows-1; i++)
				ptr[i*cols+k]=0.0;
        }
    }
    
	// 调整元素
	for (i=0; i<=rows-2; i++)
	{
		for (j=i+1; j<=rows-1;j++)
		{ 
			p=i*rows+j; 
			l=j*rows+i;
			t=mtxQ.ptr[p]; 
			mtxQ.ptr[p]=mtxQ.ptr[l]; 
			mtxQ.ptr[l]=t;
		}
	}
}

//////////////////////////////////////////////////////////////////////
// 实对称三对角阵的全部特征值与特征向量的计算
//
// 参数：
// 1. double dblB[] - 一维数组，长度为矩阵的阶数，传入对称三对角阵的主对角线元素；
//    返回时存放全部特征值。
// 2. double dblC[] - 一维数组，长度为矩阵的阶数，前n-1个元素传入对称三对角阵的次对角线元素
// 3. CCnMatrix& mtxQ - 如果传入单位矩阵，则返回实对称三对角阵的特征值向量矩阵；
//    如果传入MakeSymTri函数求得的矩阵A的豪斯荷尔德变换的乘积矩阵Q，则返回矩阵A的
//    特征值向量矩阵。其中第i列为与数组dblB中第j个特征值对应的特征向量。
// 4. int nMaxIt - 迭代次数，默认值为60
// 5. double eps - 计算精度，默认值为0.000001
//
// 返回值：BOOL型，求解是否成功
//////////////////////////////////////////////////////////////////////
void CMatrix::SymTriEigenv(double dblB[], double dblC[], 
						   CMatrix& mtxQ, int nMaxIt /*= 60*/, double eps /*= 0.000001*/)
{
	int i,j,k,m,it,u,v;
    double d,f,h,g,p,r,e,s;
    
	// 初值
	int n = mtxQ.cols;
	dblC[n-1]=0.0; 
	d=0.0; 
	f=0.0;
    
	// 迭代计算
	for (j=0; j<=n-1; j++)
    { 
		it=0;
        h=eps*(fabs(dblB[j])+fabs(dblC[j]));
        if (h>d) 
			d=h;
        
		m=j;
        while ((m<=n-1)&&(fabs(dblC[m])>d)) 
			m=m+1;
        
		if (m!=j)
        { 
			do
            { 
				assert (it!=nMaxIt);

                it=it+1;
                g=dblB[j];
                p=(dblB[j+1]-g)/(2.0*dblC[j]);
                r=sqrt(p*p+1.0);
                if (p>=0.0) 
					dblB[j]=dblC[j]/(p+r);
                else 
					dblB[j]=dblC[j]/(p-r);
                
				h=g-dblB[j];
                for (i=j+1; i<=n-1; i++)
					dblB[i]=dblB[i]-h;
                
				f=f+h; 
				p=dblB[m]; 
				e=1.0; 
				s=0.0;
                for (i=m-1; i>=j; i--)
                { 
					g=e*dblC[i]; 
					h=e*p;
                    if (fabs(p)>=fabs(dblC[i]))
                    { 
						e=dblC[i]/p; 
						r=sqrt(e*e+1.0);
                        dblC[i+1]=s*p*r; 
						s=e/r; 
						e=1.0/r;
                    }
                    else
					{ 
						e=p/dblC[i]; 
						r=sqrt(e*e+1.0);
                        dblC[i+1]=s*dblC[i]*r;
                        s=1.0/r; 
						e=e/r;
                    }
                    
					p=e*dblB[i]-s*g;
                    dblB[i+1]=h+s*(e*g+s*dblB[i]);
                    for (k=0; k<=n-1; k++)
                    { 
						u=k*n+i+1; 
						v=u-1;
                        h=mtxQ.ptr[u]; 
						mtxQ.ptr[u]=s*mtxQ.ptr[v]+e*h;
                        mtxQ.ptr[v]=e*mtxQ.ptr[v]-s*h;
                    }
                }
                
				dblC[j]=s*p; 
				dblB[j]=e*p;
            
			} while (fabs(dblC[j])>d);
        }
        
		dblB[j]=dblB[j]+f;
    }
    
	for (i=0; i<=n-1; i++)
    { 
		k=i; 
		p=dblB[i];
        if (i+1<=n-1)
        { 
			j=i+1;
            while ((j<=n-1)&&(dblB[j]<=p))
            { 
				k=j; 
				p=dblB[j]; 
				j=j+1;
			}
        }

        if (k!=i)
        { 
			dblB[k]=dblB[i]; 
			dblB[i]=p;
            for (j=0; j<=n-1; j++)
            { 
				u=j*n+i; 
				v=j*n+k;
                p=mtxQ.ptr[u]; 
				mtxQ.ptr[u]=mtxQ.ptr[v]; 
				mtxQ.ptr[v]=p;
            }
        }
    }
}

}