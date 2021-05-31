#ifndef MISC_H_
#define MISC_H_

#include <stdlib.h>

#ifdef __cplusplus
#define STB_EXTERN   extern "C"
#else
#define STB_EXTERN   extern
#endif

#define M_PI  3.14159265358979323846f

#ifndef deg2rad
#define deg2rad(a)  ((a)*(M_PI/180))
#endif
#ifndef rad2deg
#define rad2deg(a)  ((a)*(180/M_PI))
#endif

//////////////////////////////////////////////////////////////////////////////
//
//                      math/sampling operations
//

#define stb_lerp(t,a,b)               ( (a) + (t) * (float) ((b)-(a)) )
#define stb_unlerp(t,a,b)             ( ((t) - (a)) / (float) ((b) - (a)) )

#define stb_clamp(x,xmin,xmax)  ((x) < (xmin) ? (xmin) : (x) > (xmax) ? (xmax) : (x))

STB_EXTERN void stb_linear_controller(float *curpos, float target_pos,
		float acc, float deacc, float dt);

STB_EXTERN int stb_float_eq(float f1, float f2, float precision);

STB_EXTERN double stb_linear_remap(double x, double a, double b, double c,
		double d);

#ifdef STB_DEFINE

void stb_linear_controller(float *curpos, float target_pos, float acc, float deacc, float dt)
{
	float sign = 1, p, cp = *curpos;
	if (cp == target_pos) return;
	if (target_pos < cp) {
		target_pos = -target_pos;
		cp = -cp;
		sign = -1;
	}
	// first decelerate
	if (cp < 0) {
		p = cp + deacc * dt;
		if (p > 0) {
			p = 0;
			dt = dt - cp / deacc;
			if (dt < 0) dt = 0;
		} else {
			dt = 0;
		}
		cp = p;
	}
	// now accelerate
	p = cp + acc*dt;
	if (p > target_pos) p = target_pos;
	*curpos = p * sign;
	// @TODO: testing
}

int stb_float_eq(float f1, float f2, float precision)
{
	if (((f1 - precision) < f2) &&
			((f1 + precision) > f2)) {
		return 1;
	}
	return 0;
}

double stb_linear_remap(double x, double x_min, double x_max,
			double out_min, double out_max)
	{
		return stb_lerp(stb_unlerp(x,x_min,x_max),out_min,out_max);
	}
#endif

// create a macro so it's faster, but you can get at the function pointer
#define stb_linear_remap(t,a,b,c,d)   stb_lerp(stb_unlerp(t,a,b),c,d)

	STB_EXTERN int snprint_double(char *buff, uint8_t buf_len, double v,
			int decimals);

#ifdef STB_DEFINE
	int snprint_double(char *buff, uint8_t buf_len, double v, int decimals)
	{
		int i = 1;
		int int_part, fract_part;
		char *fmt = "%i.%i";
		for (; decimals != 0; i *= 10, decimals--)
		;
		int_part = (int) v;
		fract_part = (int) ((v - (double) (int) v) * i);
		if (fract_part < 0)
		fract_part *= -1;

		uint8_t size = snprintf(buff, 0, fmt, int_part, fract_part);

		if (size > buf_len) {
			for (int x = 0; x < buf_len; x++)
			*(buff++) = '#';
			*(buff++) = '\0';
			return buf_len;
		} else {
			sprintf(buff, fmt, int_part, fract_part);
			return size;
		}
	}
#endif

//////////////////////////////////////////////////////////////////////////////
//
//                         bit operations
//		http://graphics.stanford.edu/~seander/bithacks.html
//

#define stb_big32(c)    (((c)[0]<<24) + (c)[1]*65536 + (c)[2]*256 + (c)[3])
#define stb_little32(c) (((c)[3]<<24) + (c)[2]*65536 + (c)[1]*256 + (c)[0])
#define stb_big16(c)    ((c)[0]*256 + (c)[1])
#define stb_little16(c) ((c)[1]*256 + (c)[0])

	STB_EXTERN int stb_bitcount(unsigned int a);
	STB_EXTERN unsigned int stb_bitreverse8(unsigned char n);
	STB_EXTERN unsigned int stb_bitreverse(unsigned int n);

	STB_EXTERN int stb_is_pow2(size_t);
	STB_EXTERN int stb_log2_ceil(size_t);
	STB_EXTERN int stb_log2_floor(size_t);

	STB_EXTERN int stb_lowbit8(unsigned int n);
	STB_EXTERN int stb_highbit8(unsigned int n);

#ifdef STB_DEFINE
int stb_bitcount(unsigned int a)		// http://graphics.stanford.edu/~seander/bithacks.html#CountBitsSetParallel
{
   a = (a & 0x55555555) + ((a >>  1) & 0x55555555); // max 2
   a = (a & 0x33333333) + ((a >>  2) & 0x33333333); // max 4
   a = (a + (a >> 4)) & 0x0f0f0f0f; // max 8 per 4, now 8 bits
   a = (a + (a >> 8)); // max 16 per 8 bits
   a = (a + (a >> 16)); // max 32 per 8 bits
   return a & 0xff;
}

unsigned int stb_bitreverse8(unsigned char n)
{
   n = ((n & 0xAA) >> 1) + ((n & 0x55) << 1);		// swap odd and even bits
   n = ((n & 0xCC) >> 2) + ((n & 0x33) << 2);		// swap consecutive pairs
   return (unsigned char) ((n >> 4) + (n << 4));
}

unsigned int stb_bitreverse(unsigned int n)
{
  n = ((n & 0xAAAAAAAA) >>  1) | ((n & 0x55555555) << 1);	// swap odd and even bits
  n = ((n & 0xCCCCCCCC) >>  2) | ((n & 0x33333333) << 2);	// swap consecutive pairs
  n = ((n & 0xF0F0F0F0) >>  4) | ((n & 0x0F0F0F0F) << 4);	// swap nibbles ...
  n = ((n & 0xFF00FF00) >>  8) | ((n & 0x00FF00FF) << 8);	// swap bytes
  return (n >> 16) | (n << 16);
}

int stb_is_pow2(size_t n)
{
   return (n & (n-1)) == 0;
}

#endif

#endif /* MISC_H_ */
