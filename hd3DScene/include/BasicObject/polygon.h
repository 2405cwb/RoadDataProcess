//
//  TAL source code is free for non-commercial use. It may be copied,
//  modified, and redistributed provided that credit is given to the author
//  somewhere in your program documentation. TAL source code is provided
//  'as is' without any warranty, implied or expressed. TAL website:
//  http://www.agri.upm.edu.my/~chris/tal
//
//  File     : CPolygon.h
//  Author   : Christopher Teh Boon Sung (chris@agri.upm.edu.my)
//  Created  : Feb. 2001
//  Updated  : Aug. 2002
//  Version  : 1.2
//
/////////////////////////////////////////////////////////////////////////////

#ifndef TAL_POLYGON_H
#define TAL_POLYGON_H
#include "hdBasicObject.h"

#include <vector>

namespace gpc   // GPC library
{
    extern "C"
    {
#include "gpc.h"
    }
}

namespace tal
{
    // Cartesian coordinates of a point
    typedef struct POINTtag
    {
        double x, y;
        POINTtag(double dX=0.0, double dY=0.0) : x(dX), y(dY) {}
    } POINT;

    // Position of a single point in a CPolygon
    typedef struct POSITIONtag
    {
		int mPos;
        POSITIONtag() : mPos(NONE) {}
        enum {NONE=0, INTERIOR=1, EXTERIOR, VERTEX, EDGE};
    } gpcPOSITION;

    // Polygon
    class BASICOBJECT_API CPolygon
    {
    private:
        std::vector<POINT> mVertex;    // vertices

    protected:
        // convert to and from GPC:
        virtual void ToGPC(gpc::gpc_polygon *pGPC) const; 
        virtual void FromGPC(const gpc::gpc_polygon *pGPC);

        // GPC CPolygon operations (calls GPC clipping function)
        const CPolygon GPCOperation(const CPolygon &pg, gpc::gpc_op op) const;

    public:
        // Constructors and destructor:
        CPolygon();
        CPolygon(const CPolygon &rhs);
        virtual ~CPolygon();

        CPolygon &operator=(const CPolygon &rhs);

        // Accessors:
        POINT &operator[](int nIdx);
        const POINT At(int nIdx) const;
        void Set(int nIdx, POINT pt);
        void Add(POINT pt);
		void Add(double dx,double dy);
        void Del(int nIdx);
        void Clear();
        int Count() const;

        // Services:
        virtual bool IsValid() const;
        int Orientation() const;
        double Area() const;
        const gpcPOSITION Find(POINT pt) const;
        const POINT Centroid() const;
		void GetBoundBox(double& dMinx,double& dMaxx,double& dMiny,double& dMaxy);
        // Polygon clipping operations:
        const CPolygon Clip(const CPolygon &pg) const;
        const CPolygon Difference(const CPolygon &pg) const;
        const CPolygon XOR(const CPolygon &pg) const;
        const CPolygon Union(const CPolygon &pg) const;
    };
}

#endif
