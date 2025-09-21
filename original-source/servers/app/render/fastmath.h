

inline float b_sqrt(float x) {
	unsigned long	val;
	float			y,z,t;
	float	        flottant, tampon;
	
	flottant = x;
	val = *((unsigned long*)&flottant);
	val >>= 1;
	val += 0x1FC00000L;
	*((unsigned long*)&tampon) = val;
	y = tampon;
	z = y*y+x;
	t = y*y-x;
	y *= (float)4.0;
	x = z*z;
	t = t*t;
	y = z*y;
	t = (float)2.0*x-t;
	return t/y;
}

extern float b_cosinus_90(float x);
extern float b_cos(float alpha);
extern float b_sin(float alpha);
extern void b_get_cos_sin(float alpha, float *c, float *s);
