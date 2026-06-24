//
//  TAL source code is free for non-commercial use. It may be copied,
//  modified, and redistributed provided that credit is given to the author
//  somewhere in your program documentation. TAL source code is provided
//  'as is' without any warranty, implied or expressed. TAL website:
//  http://www.agri.upm.edu.my/~chris/tal
//
//  File     : futil.h
//  Author   : Christopher Teh Boon Sung (chris@agri.upm.edu.my)
//  Created  : Feb. 2001
//  Updated  : Aug. 2002
//  Version  : 1.2
//
/////////////////////////////////////////////////////////////////////////////

#ifndef TAL_FUTIL_H
#define TAL_FUTIL_H

// Floating-point operations
namespace futil
{
    const double TOL = 0.000001;     // tolerance for comparisons

    // logical comparisons:
    bool eq(double dLhs, double dRhs);          // ==
    bool neq(double dLhs, double dRhs);         // !=
    bool gt(double dLhs, double dRhs);          // >
    bool gte(double dLhs, double dRhs);         // >=
    bool lt(double dLhs, double dRhs);          // <
    bool lte(double dLhs, double dRhs);         // <=

    void round(double &dVal, int nDecimal);     // round values
    int nsgn(double dVal);                      // sign of value
}

#endif
