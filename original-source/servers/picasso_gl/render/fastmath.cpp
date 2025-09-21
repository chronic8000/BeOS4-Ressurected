
#include "fastmath.h"

/***********************************************************/
#define		SQR2	1.414213538
#ifdef PI
#undef PI
#endif
#define     PI      3.14159265358979
float b_cosinus_90(float x) {
	float			t,x4,x6;
	
	x = x*x;
	x4 = x*x;
	t = (float)SQR2-x*(float)(SQR2/8.0);
	x6 = x4*x;
	x = (float)(SQR2/46080.0)-x*(float)(SQR2/10321920.0);
	t += (float)(SQR2/384.0)*x4;
	t -= x6*x;
	return t*t-(float)1.0;
}

/***********************************************************/
float b_cos(float alpha) {
	if (alpha < 0)
		alpha = -alpha;
	if (alpha >= 2*PI)
		alpha -= (2.0*PI)*(float)((long)(alpha*(0.5/PI)));
	if (alpha > PI) alpha = (2*PI)-alpha;
	if (alpha > (PI/2.0))
		return -b_cosinus_90(PI-alpha);
	else
		return b_cosinus_90(alpha);
}
	
/***********************************************************/
float b_sin(float alpha) {
	alpha -= PI/2.0;
	if (alpha < 0)
		alpha = -alpha;
	if (alpha >= 2*PI)
		alpha -= (2.0*PI)*(float)((long)(alpha*(0.5/PI)));
	if (alpha > PI) alpha = (2*PI)-alpha;
	if (alpha > (PI/2.0))
		return -b_cosinus_90(PI-alpha);
	else
		return b_cosinus_90(alpha);
}
	
/***********************************************************/
void b_get_cos_sin(float alpha, float *c, float *s) {
	float    sgn;
	
	if (alpha < 0) {
	    alpha = -alpha;
		sgn = -1.0;
	}
	else
		sgn = 1.0;
	if (alpha >= 2*PI) {
		alpha -= (2.0*PI)*(float)((long)(alpha*(0.5/PI)));
		if (alpha < 0) {
			alpha = -alpha;
			sgn *= -1.0;
		}
	}
	if (alpha > PI) {
		if (alpha > (1.5*PI)) {
			*c = b_cosinus_90(2.0*PI-alpha);
			*s = -sgn*b_cosinus_90(alpha-1.5*PI);
		}
		else {
			*c = -b_cosinus_90(alpha-PI);
			*s = -sgn*b_cosinus_90(1.5*PI-alpha);
		}
	}
	else {
		if (alpha > (PI/2.0)) {
			*c = -b_cosinus_90(PI-alpha);
			*s = sgn*b_cosinus_90(alpha-0.5*PI);
		}
		else {
			*c = b_cosinus_90(alpha);
			*s = sgn*b_cosinus_90(0.5*PI-alpha);
		}
	}
}

/***********************************************************/
