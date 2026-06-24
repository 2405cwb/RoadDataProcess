// Copyright (C) 2002-2010 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#ifndef _HDCOLOR_H_
#define _HDCOLOR_H_

#include "hdCore.h"
#include "hdDefs.h"
#include "BaseMath.h"

namespace hd
{
	const f32 ColorRCP = 1.0f/255.0f;
	//! Creates a 16 bit A1R5G5B5 color
	inline u16 RGBA16(u32 r, u32 g, u32 b, u32 a=0xFF)
	{
		return (u16)((a & 0x80) << 8 |
			(r & 0xF8) << 7 |
			(g & 0xF8) << 2 |
			(b & 0xF8) >> 3);
	}


	//! Creates a 16 bit A1R5G5B5 color
	inline u16 RGB16(u32 r, u32 g, u32 b)
	{
		return RGBA16(r,g,b);
	}


	//! Creates a 16bit A1R5G5B5 color, based on 16bit input values
	inline u16 RGB16from16(u16 r, u16 g, u16 b)
	{
		return (0x8000 |
				(r & 0x1F) << 10 |
				(g & 0x1F) << 5  |
				(b & 0x1F));
	}


	//! Converts a 32bit (X8R8G8B8) color to a 16bit A1R5G5B5 color
	inline u16 X8R8G8B8toA1R5G5B5(u32 color)
	{
		return (u16)(0x8000 |
			( color & 0x00F80000) >> 9 |
			( color & 0x0000F800) >> 6 |
			( color & 0x000000F8) >> 3);
	}


	//! Converts a 32bit (A8R8G8B8) color to a 16bit A1R5G5B5 color
	inline u16 A8R8G8B8toA1R5G5B5(u32 color)
	{
		return (u16)(( color & 0x80000000) >> 16|
			( color & 0x00F80000) >> 9 |
			( color & 0x0000F800) >> 6 |
			( color & 0x000000F8) >> 3);
	}


	//! Converts a 32bit (A8R8G8B8) color to a 16bit R5G6B5 color
	inline u16 A8R8G8B8toR5G6B5(u32 color)
	{
		return (u16)(( color & 0x00F80000) >> 8 |
			( color & 0x0000FC00) >> 5 |
			( color & 0x000000F8) >> 3);
	}


	//! Convert A8R8G8B8 Color from A1R5G5B5 color
	/** build a nicer 32bit Color by extending dest lower bits with source high bits. */
	inline u32 A1R5G5B5toA8R8G8B8(u16 color)
	{
		return ( (( -( (s32) color & 0x00008000 ) >> (s32) 31 ) & 0xFF000000 ) |
				(( color & 0x00007C00 ) << 9) | (( color & 0x00007000 ) << 4) |
				(( color & 0x000003E0 ) << 6) | (( color & 0x00000380 ) << 1) |
				(( color & 0x0000001F ) << 3) | (( color & 0x0000001C ) >> 2)
				);
	}


	//! Returns A8R8G8B8 Color from R5G6B5 color
	inline u32 R5G6B5toA8R8G8B8(u16 color)
	{
		return 0xFF000000 |
			((color & 0xF800) << 8)|
			((color & 0x07E0) << 5)|
			((color & 0x001F) << 3);
	}


	//! Returns A1R5G5B5 Color from R5G6B5 color
	inline u16 R5G6B5toA1R5G5B5(u16 color)
	{
		return 0x8000 | (((color & 0xFFC0) >> 1) | (color & 0x1F));
	}


	//! Returns R5G6B5 Color from A1R5G5B5 color
	inline u16 A1R5G5B5toR5G6B5(u16 color)
	{
		return (((color & 0x7FE0) << 1) | (color & 0x1F));
	}



	//! Returns the alpha component from A1R5G5B5 color
	/** In Irrlicht, alpha refers to opacity.
	\return The alpha value of the color. 0 is transparent, 1 is opaque. */
	inline u32 getAlpha(u16 color)
	{
		return ((color >> 15)&0x1);
	}


	//! Returns the red component from A1R5G5B5 color.
	/** Shift left by 3 to get 8 bit value. */
	inline u32 getRed(u16 color)
	{
		return ((color >> 10)&0x1F);
	}


	//! Returns the green component from A1R5G5B5 color
	/** Shift left by 3 to get 8 bit value. */
	inline u32 getGreen(u16 color)
	{
		return ((color >> 5)&0x1F);
	}


	//! Returns the blue component from A1R5G5B5 color
	/** Shift left by 3 to get 8 bit value. */
	inline u32 getBlue(u16 color)
	{
		return (color & 0x1F);
	}


	//! Returns the average from a 16 bit A1R5G5B5 color
	inline s32 getAverage(s16 color)
	{
		return ((getRed(color)<<3) + (getGreen(color)<<3) + (getBlue(color)<<3)) / 3;
	}


	inline u32 COLORARGB(u32 a, u32 r, u32 g, u32 b)
	{
		return (((a & 0xff)<<24) | ((r & 0xff)<<16) | ((g & 0xff)<<8) | (b & 0xff));
	}

	// r,g,b values are from 0 to 1
	// h = [0,360], s = [0,1], v = [0,1]
	//		if s == 0, then h = -1 (undefined)
	inline void RGBtoHSV( float r, float g, float b, float *h, float *s, float *v )
	{
		float min, max, delta;
		min = min_<float>( r, g, b );
		max = max_<float>( r, g, b );
		*v = max;				// v
		delta = max - min;
		if( max != 0 )
			*s = delta / max;		// s
		else {
			// r = g = b = 0		// s = 0, v is undefined
			*s = 0;
			*h = -1;
			return;
		}
		if( r == max )
			*h = ( g - b ) / delta;		// between yellow & magenta
		else if( g == max )
			*h = 2 + ( b - r ) / delta;	// between cyan & yellow
		else
			*h = 4 + ( r - g ) / delta;	// between magenta & cyan
		*h *= 60;				// degrees
		if( *h < 0 )
			*h += 360;
	}
	// r,g,b values are from 0 to 1
	// h = [0,360], s = [0,1], v = [0,1]
	//		if s == 0, then h = -1 (undefined)
	inline void HSVtoRGB( float *r, float *g, float *b, float h, float s, float v )
	{
		int i;
		float f, p, q, t;
		if( s == 0 ) {
			// achromatic (grey)
			*r = *g = *b = v;
			return;
		}
		h /= 60;			// sector 0 to 5
		i = (int)floor( h );
		f = h - i;			// factorial part of h
		p = v * ( 1 - s );
		q = v * ( 1 - s * f );
		t = v * ( 1 - s * ( 1 - f ) );
		switch( i ) {
		case 0:
			*r = v;
			*g = t;
			*b = p;
			break;
		case 1:
			*r = q;
			*g = v;
			*b = p;
			break;
		case 2:
			*r = p;
			*g = v;
			*b = t;
			break;
		case 3:
			*r = p;
			*g = q;
			*b = v;
			break;
		case 4:
			*r = t;
			*g = p;
			*b = v;
			break;
		default:		// case 5:
			*r = v;
			*g = p;
			*b = q;
			break;
		}
	}

	//! RGB888转换YUV
	inline void RGBtoYUV(u8 r,u8 g,u8 b,u8& y,u8& u,u8& v)
	{
		y = (( 66 * r + 129 * g +  25 * b + 128) >> 8) + 16;
		u = ((-38 * r -  74 * g + 112 * b + 128) >> 8) + 128;
		v = ((112 * r -  79 * g -  18 * b + 128) >> 8) + 128;
	}
	//! Class representing a 32 bit ARGB color.
	/** The color values for alpha, red, green, and blue are
	stored in a single u32. So all four values may be between 0 and 255.
	Alpha in Irrlicht is opacity, so 0 is fully transparent, 255 is fully opaque (solid).
	This class is used by most parts of the Irrlicht Engine
	to specify a color. Another way is using the class SColorf, which
	stores the color values in 4 floats.
	This class must consist of only one u32 and must not use virtual functions.
	*/
	class CHdColor
	{
	public:

		//! Constructor of the Color. Does nothing.
		/** The color value is not initialized to save time. */
		CHdColor() {}

		//! Constructs the color from 4 values representing the alpha, red, green and blue component.
		/** Must be values between 0 and 255. */
		CHdColor (u32 a, u32 r, u32 g, u32 b)
			: color(((a & 0xff)<<24) | ((r & 0xff)<<16) | ((g & 0xff)<<8) | (b & 0xff)) {}

		//! Constructs the color from a 32 bit value. Could be another color.
		CHdColor(u32 clr)
			: color(clr) {}

		//! Returns the alpha component of the color.
		/** The alpha component defines how opaque a color is.
		\return The alpha value of the color. 0 is fully transparent, 255 is fully opaque. */
		u32 getAlpha() const { return color>>24; }

		//! Returns the red component of the color.
		/** \return Value between 0 and 255, specifying how red the color is.
		0 means no red, 255 means full red. */
		u32 getRed() const { return (color>>16) & 0xff; }

		//! Returns the green component of the color.
		/** \return Value between 0 and 255, specifying how green the color is.
		0 means no green, 255 means full green. */
		u32 getGreen() const { return (color>>8) & 0xff; }

		//! Returns the blue component of the color.
		/** \return Value between 0 and 255, specifying how blue the color is.
		0 means no blue, 255 means full blue. */
		u32 getBlue() const { return color & 0xff; }

		//! Get lightness of the color in the range [0,255]
		f32 getLightness() const
		{
			return 0.5f*(max_(max_(getRed(),getGreen()),getBlue()) + min_(min_(getRed(),getGreen()),getBlue()));
		}

		//! Get luminance of the color in the range [0,255].
		f32 getLuminance() const
		{
			return 0.3f*getRed() + 0.59f*getGreen() + 0.11f*getBlue();
		}

		//! Get average intensity of the color in the range [0,255].
		u32 getAverage() const
		{
			return ( getRed() + getGreen() + getBlue() ) / 3;
		}

		//! Sets the alpha component of the Color.
		/** The alpha component defines how transparent a color should be.
		\param a The alpha value of the color. 0 is fully transparent, 255 is fully opaque. */
		void setAlpha(u32 a) { color = ((a & 0xff)<<24) | (color & 0x00ffffff); }

		//! Sets the red component of the Color.
		/** \param r: Has to be a value between 0 and 255.
		0 means no red, 255 means full red. */
		void setRed(u32 r) { color = ((r & 0xff)<<16) | (color & 0xff00ffff); }

		//! Sets the green component of the Color.
		/** \param g: Has to be a value between 0 and 255.
		0 means no green, 255 means full green. */
		void setGreen(u32 g) { color = ((g & 0xff)<<8) | (color & 0xffff00ff); }

		//! Sets the blue component of the Color.
		/** \param b: Has to be a value between 0 and 255.
		0 means no blue, 255 means full blue. */
		void setBlue(u32 b) { color = (b & 0xff) | (color & 0xffffff00); }

		//! Calculates a 16 bit A1R5G5B5 value of this color.
		/** \return 16 bit A1R5G5B5 value of this color. */
		u16 toA1R5G5B5() const { return A8R8G8B8toA1R5G5B5(color); }

		//! Converts color to OpenGL color format
		/** From ARGB to RGBA in 4 byte components for endian aware
		passing to OpenGL
		\param dest: address where the 4x8 bit OpenGL color is stored. */
		void toOpenGLColor(u8* dest) const
		{
			*dest =   (u8)getRed();
			*++dest = (u8)getGreen();
			*++dest = (u8)getBlue();
			*++dest = (u8)getAlpha();
		}

		//! Sets all four components of the color at once.
		/** Constructs the color from 4 values representing the alpha,
		red, green and blue components of the color. Must be values
		between 0 and 255.
		\param a: Alpha component of the color. The alpha component
		defines how transparent a color should be. Has to be a value
		between 0 and 255. 255 means not transparent (opaque), 0 means
		fully transparent.
		\param r: Sets the red component of the Color. Has to be a
		value between 0 and 255. 0 means no red, 255 means full red.
		\param g: Sets the green component of the Color. Has to be a
		value between 0 and 255. 0 means no green, 255 means full
		green.
		\param b: Sets the blue component of the Color. Has to be a
		value between 0 and 255. 0 means no blue, 255 means full blue. */
		void set(u32 a, u32 r, u32 g, u32 b)
		{
			color = (((a & 0xff)<<24) | ((r & 0xff)<<16) | ((g & 0xff)<<8) | (b & 0xff));
		}
		void set(u32 col) { color = col; }

		//! Compares the color to another color.
		/** \return True if the colors are the same, and false if not. */
		bool operator==(const CHdColor& other) const { return other.color == color; }

		//! Compares the color to another color.
		/** \return True if the colors are different, and false if they are the same. */
		bool operator!=(const CHdColor& other) const { return other.color != color; }

		//! comparison operator
		/** \return True if this color is smaller than the other one */
		bool operator<(const CHdColor& other) const { return (color < other.color); }

		//! Adds two colors, result is clamped to 0..255 values
		/** \param other Color to add to this color
		\return Addition of the two colors, clamped to 0..255 values */
		CHdColor operator+(const CHdColor& other) const
		{
			return CHdColor(MIN(getAlpha() + other.getAlpha(), 255u),
					MIN(getRed() + other.getRed(), 255u),
					MIN(getGreen() + other.getGreen(), 255u),
					MIN(getBlue() + other.getBlue(), 255u));
		}

		//! Interpolates the color with a f32 value to another color
		/** \param other: Other color
		\param d: value between 0.0f and 1.0f
		\return Interpolated color. */
		CHdColor getInterpolated(const CHdColor &other, f32 d) const
		{
			d = clamp(d, 0.f, 1.f);
			const f32 inv = 1.0f - d;
			return CHdColor((u32)hd_lrintf(other.getAlpha()*inv + getAlpha()*d),
				(u32)hd_lrintf(other.getRed()*inv + getRed()*d),
				(u32)hd_lrintf(other.getGreen()*inv + getGreen()*d),
				(u32)hd_lrintf(other.getBlue()*inv + getBlue()*d));
		}

		//! Returns interpolated color. ( quadratic )
		/** \param c1: first color to interpolate with
		\param c2: second color to interpolate with
		\param d: value between 0.0f and 1.0f. */
		//SColor getInterpolated_quadratic(const SColor& c1, const SColor& c2, f32 d) const
		//{
		//	// this*(1-d)*(1-d) + 2 * c1 * (1-d) + c2 * d * d;
		//	d = clamp(d, 0.f, 1.f);
		//	const f32 inv = 1.f - d;
		//	const f32 mul0 = inv * inv;
		//	const f32 mul1 = 2.f * d * inv;
		//	const f32 mul2 = d * d;

		//	return SColor(
		//			clamp( I32_FLOOR(
		//					getAlpha() * mul0 + c1.getAlpha() * mul1 + c2.getAlpha() * mul2 ), 0, 255 ),
		//			clamp( I32_FLOOR(
		//					getRed()   * mul0 + c1.getRed()   * mul1 + c2.getRed()   * mul2 ), 0, 255 ),
		//			clamp ( I32_FLOOR(
		//					getGreen() * mul0 + c1.getGreen() * mul1 + c2.getGreen() * mul2 ), 0, 255 ),
		//			clamp ( I32_FLOOR(
		//					getBlue()  * mul0 + c1.getBlue()  * mul1 + c2.getBlue()  * mul2 ), 0, 255 ));
		//}

		//! color in A8R8G8B8 Format
		u32 color;
	};


	//! Class representing a color with four floats.
	/** The color values for red, green, blue
	and alpha are each stored in a 32 bit floating point variable.
	So all four values may be between 0.0f and 1.0f.
	Another, faster way to define colors is using the class SColor, which
	stores the color values in a single 32 bit integer.
	*/
	class CHdColorf
	{
	public:
		//! Default constructor for SColorf.
		/** Sets red, green and blue to 0.0f and alpha to 1.0f. */
		CHdColorf() : r(0.0f), g(0.0f), b(0.0f), a(1.0f) {}

		//! Constructs a color from up to four color values: red, green, blue, and alpha.
		/** \param r: Red color component. Should be a value between
		0.0f meaning no red and 1.0f, meaning full red.
		\param g: Green color component. Should be a value between 0.0f
		meaning no green and 1.0f, meaning full green.
		\param b: Blue color component. Should be a value between 0.0f
		meaning no blue and 1.0f, meaning full blue.
		\param a: Alpha color component of the color. The alpha
		component defines how transparent a color should be. Has to be
		a value between 0.0f and 1.0f, 1.0f means not transparent
		(opaque), 0.0f means fully transparent. */
		CHdColorf(f32 r, f32 g, f32 b, f32 a = 1.0f) : r(r), g(g), b(b), a(a) {}

		//! Constructs a color from 32 bit Color.
		/** \param c: 32 bit color from which this SColorf class is
		constructed from. */
		CHdColorf(CHdColor c)
		{
			const f32 inv = 1.0f / 255.0f;
			r = c.getRed() * inv;
			g = c.getGreen() * inv;
			b = c.getBlue() * inv;
			a = c.getAlpha() * inv;
		}

		//! Converts this color to a SColor without floats.
		CHdColorf toSColor() const
		{
			return CHdColor((u32)hd_lrintf(a*255.0f), (u32)hd_lrintf(r*255.0f), (u32)hd_lrintf(g*255.0f), (u32)hd_lrintf(b*255.0f));
		}

		//! Sets three color components to new values at once.
		/** \param rr: Red color component. Should be a value between 0.0f meaning
		no red (=black) and 1.0f, meaning full red.
		\param gg: Green color component. Should be a value between 0.0f meaning
		no green (=black) and 1.0f, meaning full green.
		\param bb: Blue color component. Should be a value between 0.0f meaning
		no blue (=black) and 1.0f, meaning full blue. */
		void set(f32 rr, f32 gg, f32 bb) {r = rr; g =gg; b = bb; }

		//! Sets all four color components to new values at once.
		/** \param aa: Alpha component. Should be a value between 0.0f meaning
		fully transparent and 1.0f, meaning opaque.
		\param rr: Red color component. Should be a value between 0.0f meaning
		no red and 1.0f, meaning full red.
		\param gg: Green color component. Should be a value between 0.0f meaning
		no green and 1.0f, meaning full green.
		\param bb: Blue color component. Should be a value between 0.0f meaning
		no blue and 1.0f, meaning full blue. */
		void set(f32 aa, f32 rr, f32 gg, f32 bb) {a = aa; r = rr; g =gg; b = bb; }

		//! Interpolates the color with a f32 value to another color
		/** \param other: Other color
		\param d: value between 0.0f and 1.0f
		\return Interpolated color. */
		CHdColorf getInterpolated(const CHdColorf &other, f32 d) const
		{
			d = clamp(d, 0.f, 1.f);
			const f32 inv = 1.0f - d;
			return CHdColorf(other.r*inv + r*d,
				other.g*inv + g*d, other.b*inv + b*d, other.a*inv + a*d);
		}

		//! Returns interpolated color. ( quadratic )
		/** \param c1: first color to interpolate with
		\param c2: second color to interpolate with
		\param d: value between 0.0f and 1.0f. */
		inline CHdColorf getInterpolated_quadratic(const CHdColorf& c1, const CHdColorf& c2,
				f32 d) const
		{
			d = clamp(d, 0.f, 1.f);
			// this*(1-d)*(1-d) + 2 * c1 * (1-d) + c2 * d * d;
			const f32 inv = 1.f - d;
			const f32 mul0 = inv * inv;
			const f32 mul1 = 2.f * d * inv;
			const f32 mul2 = d * d;

			return CHdColorf (r * mul0 + c1.r * mul1 + c2.r * mul2,
					g * mul0 + c1.g * mul1 + c2.g * mul2,
					b * mul0 + c1.b * mul1 + c2.b * mul2,
					a * mul0 + c1.a * mul1 + c2.a * mul2);
		}


		//! Sets a color component by index. R=0, G=1, B=2, A=3
		void setColorComponentValue(s32 index, f32 value)
		{
			switch(index)
			{
			case 0: r = value; break;
			case 1: g = value; break;
			case 2: b = value; break;
			case 3: a = value; break;
			}
		}

		//! Returns the alpha component of the color in the range 0.0 (transparent) to 1.0 (opaque)
		f32 getAlpha() const { return a; }

		//! Returns the red component of the color in the range 0.0 to 1.0
		f32 getRed() const { return r; }

		//! Returns the green component of the color in the range 0.0 to 1.0
		f32 getGreen() const { return g; }

		//! Returns the blue component of the color in the range 0.0 to 1.0
		f32 getBlue() const { return b; }

		//! red color component
		f32 r;

		//! green color component
		f32 g;

		//! blue component
		f32 b;

		//! alpha color component
		f32 a;
	};


	//! Class representing a color in HSV format
	/** The color values for hue, saturation, value
	are stored in a 32 bit floating point variable.
	*/
	class CHdColorHSL
	{
	public:
		CHdColorHSL ( f32 h = 0.f, f32 s = 0.f, f32 l = 0.f )
			: Hue ( h ), Saturation ( s ), Luminance ( l ) {}

		void fromRGB(const CHdColor &color);
		void toRGB(CHdColor &color) const;

		f32 Hue;
		f32 Saturation;
		f32 Luminance;

	private:
		inline u32 toRGB1(f32 rm1, f32 rm2, f32 rh) const;

	};

	static COLORREF colorRampList[5][11] = 
	{
		{
			RGB(202,111,255),
			RGB( 150, 111, 255 ),
			RGB( 70, 111, 255 ),
			RGB( 0, 111, 255 ),
			RGB( 0, 50, 255 ),
			RGB( 0, 0, 255 ),
			RGB( 0, 0, 150 ),
			RGB( 0, 70, 150 ),
			RGB( 0, 150, 150 ),
			RGB( 0, 255, 150 ),
			RGB(0,255,255)
		},
		{
			RGB(0,255,0),
			RGB( 0, 255, 70 ),
			RGB( 0, 255, 150 ),
			RGB( 0, 150, 150 ),
			RGB( 0, 70, 150 ),
			RGB( 0, 0, 255 ),
			RGB( 70, 0, 255 ),
			RGB( 150, 0, 255 ),
			RGB( 255, 0, 150 ),
			RGB( 255, 0, 0 ),
			RGB(255,0,0)
		},
		{
			RGB(255,0,0),
			RGB( 255, 180, 0 ),
			RGB( 255, 120, 0 ),
			RGB( 255, 0, 0 ),
			RGB( 255, 0, 80 ),
			RGB( 255, 0, 150 ),
			RGB( 255, 120, 150 ),
			RGB( 180, 120, 150 ),
			RGB( 120, 120, 80 ),
			RGB( 80, 180, 80 ),
			RGB(0,255,0)
		},
		{
			RGB(245,245,150),
			RGB( 250, 245,120),
			RGB( 255, 255, 60 ),
			RGB( 255, 220, 0 ),
			RGB( 255, 120, 0 ),
			RGB( 255, 0, 60 ),
			RGB( 255, 0, 150 ),
			RGB( 220, 0, 240 ),
			RGB( 150, 20, 220 ),
			RGB( 75, 30, 200 ),
			RGB(20,20,175)
		},
		{
			RGB(0,0,180),
			RGB( 0, 120, 255 ),
			RGB( 0, 220, 255 ),
			RGB( 0, 220, 120 ),
			RGB( 0, 220, 0 ),
			RGB( 120, 220, 0 ),
			RGB( 255, 220, 0 ),
			RGB( 255, 120, 0 ),
			RGB( 255, 90, 0 ),
			RGB( 255, 60, 0 ),
			RGB(180,0,0)
		},
	};

	class CHdColorRamp 
	{
	private:
		//! 透明度差值
		s32  m_difA;
		//! R差值
		s32  m_difR;
		//! G差值
		s32  m_difG;
		//! B差值
		s32  m_difB;
	private:
		s32 m_begA;
		s32 m_begR;
		s32 m_begG;
		s32 m_begB;
		s32 m_tempA[10];
		s32 m_tempR[10];
		s32 m_tempG[10];
		s32 m_tempB[10];		

		//! 起始颜色,A8R8G8B8格式
		u32 m_colorBegin;
		u32 m_colorEnd;
		int m_nSelCursel;
	public:		
		//! 构造函数
		CHdColorRamp(u32 beginClr,u32 endClr)
			:m_colorBegin(beginClr)//,m_colorEnd(endClr)
		{
			m_begA = (beginClr>>24);
			m_begR = ((beginClr>>16) & 0xff);
			m_begG = ((beginClr>>8) & 0xff);
			m_begB = (beginClr & 0xff);

			m_difA = (endClr>>24) - m_begA;
			m_difR = ((endClr>>16) & 0xff) - m_begR;
			m_difG = ((endClr>>8) & 0xff) - m_begG;
			m_difB = ((endClr) & 0xff) - m_begB;
		}

		bool operator==(const CHdColorRamp& other) const 
		{ 
			return other.m_colorBegin == m_colorBegin && other.m_colorEnd == m_colorEnd; 
		}
		//! 返回渐变色,scale在0.0f到1.0f之间
		inline void GetColor4f(f32 scale,f32& a,f32& r,f32& g,f32& b)
		{
			scale = clamp(scale, 0.f, 1.f);
			a = (scale * m_difA + m_begA) * ColorRCP;
			r = (scale * m_difR + m_begR) * ColorRCP;
			g = (scale * m_difG + m_begG) * ColorRCP;
			b = (scale * m_difB + m_begB) * ColorRCP;
		}

		//! 构造函数，渐变颜色为10段渐变
		CHdColorRamp(u32 beginClr,u32 endClr,int nStep)
			:m_colorBegin(beginClr),m_colorEnd(endClr)
		{
			m_begA = (beginClr>>24);
			m_begR = ((beginClr>>16) & 0xff);
			m_begG = ((beginClr>>8) & 0xff);
			m_begB = (beginClr & 0xff);

			for (int i = 0;i < 10;i++)
			{
				m_tempA[i] = 255;
			}
			m_tempR[0] = 0; m_tempR[1] = 0; m_tempR[2] = 0; m_tempR[3] = 0; m_tempR[4] = 120;
			m_tempR[5] = 255; m_tempR[6] = 255; m_tempR[7] = 255; m_tempR[8] = 255;
			m_tempG[0] = 120; m_tempG[1] = 220; m_tempG[2] = 220; m_tempG[3] = 220; m_tempG[4] = 220;
			m_tempG[5] = 220; m_tempG[6] = 120; m_tempG[7] = 90; m_tempG[8] = 60;
			m_tempB[0] = 255; m_tempB[1] = 255; m_tempB[2] = 120; m_tempB[3] = 0; m_tempB[4] = 0;
			m_tempB[5] = 0; m_tempB[6] = 0; m_tempB[7] = 0; m_tempB[8] = 0;

			m_tempA[9] = (m_colorEnd>>24);
			m_tempR[9] = ((m_colorEnd>>16) & 0xff);
			m_tempG[9] = ((m_colorEnd>>8) & 0xff);
			m_tempB[9] = (m_colorEnd & 0xff);

			m_nSelCursel = 4;
		}

		//! 10段渐变色rgb颜色获取
		inline void GetColor4f(f32 scale,f32& a,f32& r,f32& g,f32& b,int step)
		{
			scale = clamp(scale, 0.f, 1.f);
			step = clamp(step, 1, 10);

			if (step == 1)
			{
				a = (scale * (m_tempA[0] - m_begA) + m_begA) * ColorRCP;
				r = (scale * (m_tempR[0] - m_begR) + m_begR) * ColorRCP;
				g = (scale * (m_tempG[0] - m_begG) + m_begG) * ColorRCP;
				b = (scale * (m_tempB[0] - m_begB) + m_begB) * ColorRCP;
			}
			else
			{
				a = (scale * (m_tempA[step-1] - m_tempA[step-2]) + m_tempA[step-2]) * ColorRCP;
				r = (scale * (m_tempR[step-1] - m_tempR[step-2]) + m_tempR[step-2]) * ColorRCP;
				g = (scale * (m_tempG[step-1] - m_tempG[step-2]) + m_tempG[step-2]) * ColorRCP;
				b = (scale * (m_tempB[step-1] - m_tempB[step-2]) + m_tempB[step-2]) * ColorRCP;
			}
		}

		//! 10段渐变色rgb颜色获取（8位）
		inline void GetColor4ub(f32 scale,u8& a,u8& r,u8& g,u8& b,int step)
		{
			scale = clamp(scale, 0.f, 1.f);
			step = clamp(step, 1, 10);

			if (step == 1)
			{
				a = (u8)(scale * (m_tempA[0] - m_begA) + m_begA);
				r = (u8)(scale * (m_tempR[0] - m_begR) + m_begR);
				g = (u8)(scale * (m_tempG[0] - m_begG) + m_begG);
				b = (u8)(scale * (m_tempB[0] - m_begB) + m_begB);
			}
			else
			{
				a = (u8)(scale * (m_tempA[step-1] - m_tempA[step-2]) + m_tempA[step-2]);
				r = (u8)(scale * (m_tempR[step-1] - m_tempR[step-2]) + m_tempR[step-2]);
				g = (u8)(scale * (m_tempG[step-1] - m_tempG[step-2]) + m_tempG[step-2]);
				b = (u8)(scale * (m_tempB[step-1] - m_tempB[step-2]) + m_tempB[step-2]);
			}
		}

		//! 10段渐变色rgb颜色获取（32位）
		inline void GetColor4i(f32 scale,u32& a,u32& r,u32& g,u32& b,int step)
		{
			scale = clamp(scale, 0.f, 1.f);
			step = clamp(step, 1, 10);

			if (step == 1)
			{
				a = (u32)(scale * (m_tempA[0] - m_begA) + m_begA);
				r = (u32)(scale * (m_tempR[0] - m_begR) + m_begR);
				g = (u32)(scale * (m_tempG[0] - m_begG) + m_begG);
				b = (u32)(scale * (m_tempB[0] - m_begB) + m_begB);
			}
			else
			{
				a = (u32)(scale * (m_tempA[step-1] - m_tempA[step-2]) + m_tempA[step-2]);
				r = (u32)(scale * (m_tempR[step-1] - m_tempR[step-2]) + m_tempR[step-2]);
				g = (u32)(scale * (m_tempG[step-1] - m_tempG[step-2]) + m_tempG[step-2]);
				b = (u32)(scale * (m_tempB[step-1] - m_tempB[step-2]) + m_tempB[step-2]);
			}
		}


		//! 根据索引值0-4分别取对应的颜色段
		inline void SetRampColor4f(int cursel)
		{
			m_begA = 255;
			m_begR = GetRValue(colorRampList[cursel][0]);
			m_begG = GetGValue(colorRampList[cursel][0]);
			m_begB = GetBValue(colorRampList[cursel][0]);

			m_colorBegin = COLORARGB(m_begA,m_begR,m_begG,m_begB);

			for (int i = 1;i < 11;i++)
			{
				m_tempA[i-1] = 255;
				m_tempR[i-1] = GetRValue(colorRampList[cursel][i]);
				m_tempG[i-1] = GetGValue(colorRampList[cursel][i]);
				m_tempB[i-1] = GetBValue(colorRampList[cursel][i]);
			}
			m_colorEnd = COLORARGB(m_tempA[9],m_tempR[9],m_tempG[9],m_tempB[9]);
			m_nSelCursel = cursel;
		}

		//! 10段渐变色获得索引值0-4范围内
		inline int GetCurCursel() const{return m_nSelCursel;}

		//! 10段渐变色设置起始、终止颜色值
		inline void SetBegColor4f(u32 begColor,u32 endColor)
		{
			m_colorBegin = begColor;
			m_colorEnd = endColor;

			m_begA = (begColor>>24);
			m_begR = ((begColor>>16) & 0xff);
			m_begG = ((begColor>>8) & 0xff);
			m_begB = (begColor & 0xff);

			for (int i = 0;i < 10;i++)
			{
				m_tempA[i] = 255;
			}
			m_tempR[0] = 0; m_tempR[1] = 0; m_tempR[2] = 0; m_tempR[3] = 0; m_tempR[4] = 120;
			m_tempR[5] = 255; m_tempR[6] = 255; m_tempR[7] = 255; m_tempR[8] = 255;
			m_tempG[0] = 120; m_tempG[1] = 220; m_tempG[2] = 220; m_tempG[3] = 220; m_tempG[4] = 220;
			m_tempG[5] = 220; m_tempG[6] = 120; m_tempG[7] = 90; m_tempG[8] = 60;
			m_tempB[0] = 255; m_tempB[1] = 255; m_tempB[2] = 120; m_tempB[3] = 0; m_tempB[4] = 0;
			m_tempB[5] = 0; m_tempB[6] = 0; m_tempB[7] = 0; m_tempB[8] = 0;

			m_tempA[9] = (endColor>>24);
			m_tempR[9] = ((endColor>>16) & 0xff);
			m_tempG[9] = ((endColor>>8) & 0xff);
			m_tempB[9] = (endColor & 0xff);
		}

		//! 10段获取渐变色起始颜色值
		inline u32 GetBeginColor() const{return m_colorBegin;}

		//! 10段获取渐变色终止颜色值
		inline u32 GetEndColor()const{return m_colorEnd;}
	};

	inline void CHdColorHSL::fromRGB(const CHdColor &color)
	{		
		const u32 maxValInt = max_(color.getRed(), color.getGreen(), color.getBlue());
		const f32 maxVal = (f32)maxValInt;
		const f32 minVal = (f32)min_(color.getRed(), color.getGreen(), color.getBlue());
		Luminance = (maxVal/minVal)*0.5f;
		if (/*core::equals(maxVal, minVal)*/maxVal == minVal)
		{
			Hue=0.f;
			Saturation=0.f;
			return;
		}

		const f32 delta = maxVal-minVal;
		if ( Luminance <= 0.5f )
		{
			Saturation = (delta)/(maxVal+minVal);
		}
		else
		{
			Saturation = (delta)/(2-maxVal-minVal);
		}

		if (maxValInt == color.getRed())
			Hue = (color.getGreen()-color.getBlue())/delta;
		else if (maxValInt == color.getGreen())
			Hue = 2+(color.getBlue()-color.getRed())/delta;
		else // blue is max
			Hue = 4+(color.getRed()-color.getGreen())/delta;

		Hue *= (60.0f * DEGTORAD);
		while ( Hue < 0.f )
			Hue += (f32)(2.f * PI64);
	}


	inline void CHdColorHSL::toRGB(CHdColor &color) const
	{
		if (Saturation == 0) // grey
		{
			u8 c = (u8) ( Luminance * 255.0 );
			color.setRed(c);
			color.setGreen(c);
			color.setBlue(c);
			return;
		}

		f32 rm2;

		if ( Luminance <= 0.5f )
		{
			rm2 = Luminance + Luminance * Saturation;
		}
		else
		{
			rm2 = Luminance + Saturation - Luminance * Saturation;
		}

		const f32 rm1 = 2.0f * Luminance - rm2;

		color.setRed ( toRGB1(rm1, rm2, Hue + (120.0f * DEGTORAD )) );
		color.setGreen ( toRGB1(rm1, rm2, Hue) );
		color.setBlue ( toRGB1(rm1, rm2, Hue - (120.0f * DEGTORAD) ) );
	}


	inline u32 CHdColorHSL::toRGB1(f32 rm1, f32 rm2, f32 rh) const
	{
		while ( rh > 2.f * PI64 )
			rh -= (f32)(2.f * PI64);

		while ( rh < 0.f )
			rh += (f32)(2.f * PI64);

		if (rh < 60.0f * DEGTORAD )
			rm1 = rm1 + (rm2 - rm1) * rh / (60.0f * DEGTORAD);
		else if (rh < 180.0f * DEGTORAD )
			rm1 = rm2;
		else if (rh < 240.0f * DEGTORAD )
			rm1 = rm1 + (rm2 - rm1) * ( ( 240.0f * DEGTORAD ) - rh) /
				(60.0f * DEGTORAD);

		return (u32) hd_lrintf(rm1 * 255.f);
	}

} // end namespace hd

#endif

