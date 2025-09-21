
//******************************************************************************
//
//	File:		bezier.cpp
//
//	Description:	Tesselation of cubic bezier curves
//	
//	Written by:	George Hoffman
//
//	Copyright 1997, Be Incorporated
//
//******************************************************************************/

#include <math.h>
#include <Debug.h>
#include "bezier.h"
#include "pointInlines.h"

#define BEZIER_LEFT 1
#define BEZIER_RIGHT 2
#define BEZIER_DONE 3

inline bool Colinear(fpoint p0, fpoint p1, fpoint p2, fpoint p3, float maxError2)
{
	fpoint d,p,_d;
	float len;
	
	d.h = p3.h-p0.h;
	d.v = p3.v-p0.v;
	
	_d.h = -d.v;
	_d.v = d.h;
	FindIntersection(p0,d,p1,_d,p);
	p.h = p1.h-p.h;
	p.v = p1.v-p.v;
	len = p.h*p.h + p.v*p.v;
	
	if (len < maxError2) {
		FindIntersection(p0,d,p2,_d,p);
		p.h = p2.h-p.h;
		p.v = p2.v-p.v;
		len = p.h*p.h + p.v*p.v;
		if (len < maxError2)
			return true;
	};
	
	return false;
};

struct BezierStackFrame {
	fpoint center,l1,r1,l2,r2,p0,p1,p2,p3;
	int8 state,needClipping;
};

#define BEZIER_STACK_SIZE 16

void TesselateClippedCubicBezier(
		fpoint *p, BArray<fpoint> &curve,
		frect clip, float maxError)
{
	BezierStackFrame stackFrames[BEZIER_STACK_SIZE],*stack,*oldStack;
	fpoint tmp;
	frect bound;
	float maxError2 = maxError*maxError;
	int32 stackCount=0;
	int8 needClipping,clipped;

	/*	Here's how this recursion works.
	
		p0 through p3 define the control points at the current
		level.  l1,l2,r1,r2 define the middle two control points of,
		respectively, the left half and right half beziers.  center
		is the rightmost control point for the left half and the leftmost
		for the right half, as well as being the point on the curve that
		this recursion contributes.  "state" is the three-valued state of
		recursion.  If it is LEFT, then we should recurse left.  If it is
		RIGHT, recurse right.  If it is DONE, we should go up a level.

		Note that this methodology means that p0 through p3 for a given
		stackPtr=n should be set by level n-1.  Function arguments, if
		you will.
	*/

	stack = stackFrames;
	
	stack->p0 = p[0];
	stack->p1 = p[1];
	stack->p2 = p[2];
	stack->p3 = p[3];
	stack->state = BEZIER_LEFT;
	stack->needClipping = 1;

loop:

	clipped = 0;
	needClipping = stack->needClipping;
	if (needClipping) {
		bound.left = bound.right = stack->p0.h;
		bound.top = bound.bottom = stack->p0.v;
		if (stack->p1.h < bound.left) bound.left = stack->p1.h;
		if (stack->p2.h < bound.left) bound.left = stack->p2.h;
		if (stack->p3.h < bound.left) bound.left = stack->p3.h;
		if (stack->p1.h > bound.right) bound.right = stack->p1.h;
		if (stack->p2.h > bound.right) bound.right = stack->p2.h;
		if (stack->p3.h > bound.right) bound.right = stack->p3.h;
		if (stack->p1.v < bound.top) bound.top = stack->p1.v;
		if (stack->p2.v < bound.top) bound.top = stack->p2.v;
		if (stack->p3.v < bound.top) bound.top = stack->p3.v;
		if (stack->p1.v > bound.bottom) bound.bottom = stack->p1.v;
		if (stack->p2.v > bound.bottom) bound.bottom = stack->p2.v;
		if (stack->p3.v > bound.bottom) bound.bottom = stack->p3.v;
		needClipping = 0;
		if (clip.left > bound.left) { needClipping = 1; bound.left = clip.left; };
		if (clip.top > bound.top) { needClipping = 1; bound.top = clip.top; };
		if (clip.right < bound.right) { needClipping = 1; bound.right = clip.right; };
		if (clip.bottom < bound.bottom) { needClipping = 1; bound.bottom = clip.bottom; };
		clipped = (bound.left >= bound.right) || (bound.top >= bound.bottom);
	};
	
	/*	We want to stop the recursion if the control points are
		approximately colinear.  Just check if p1 and p2 are "close" to
		being on the line connecting p0 and p3. */

	if ((stackCount > (BEZIER_STACK_SIZE-1)) ||
		Colinear(stack->p0,stack->p1,stack->p2,stack->p3,maxError2) ||
		clipped) {
		stackCount--;
		stack--;
		goto unrecurse;
	};

	/*	We need more precision.  First compute the point-on-curve
		and left and right half control points and store them in the
		stack. */
	tmp				= MidPoint(stack->p1,stack->p2);
	stack->l1		= MidPoint(stack->p0,stack->p1);
	stack->r1 		= MidPoint(stack->p2,stack->p3);
	stack->l2		= MidPoint(stack->l1,tmp);
	stack->r2		= MidPoint(tmp,stack->r1);
	stack->center	= MidPoint(stack->l2,stack->r2);
	
	/*	Now we're ready to recurse left. */
	oldStack = stack++;
	stackCount++;
	stack->p0 = oldStack->p0;		// Leftmost point doesn't change
	stack->p1 = oldStack->l1;		// l1 and l2 define the middle two
	stack->p2 = oldStack->l2;		//    control points for the left half
	stack->p3 = oldStack->center;	// center is the rightmost point of left half
	oldStack->state = BEZIER_RIGHT;	// Remind ourselves to go right next
	stack->state = BEZIER_LEFT;		// Tell the child to start by going left
	stack->needClipping = needClipping;
	goto loop;

unrecurse:
			
	/*	If we're here we've just come back up from a child recursion.
		We have to check the stack state to see if we still have to recurse
		right or if we're done. */

	if ((stackCount<0) || (stack->state==BEZIER_DONE)) {
		/*	We're done with this level.  If this is the top level,
			the whole curve is done except for the last point, so
			exit the loop.  Otherwise, unrecurse. */
		if (stackCount<=0)
			goto doneRecursion;
		stackCount--;
		stack--;
		goto unrecurse;
	};
	
	/*	Before we recurse right, we have to add our center point to the curve. */
	curve.AddItem(stack->center);

	/*	Recurse right */
	oldStack = stack++;
	stackCount++;
	stack->p0 = oldStack->center;	// center is the leftmost point of right half
	stack->p1 = oldStack->r2;		// r1 and r2 define the middle two
	stack->p2 = oldStack->r1;		//    control points for the right half
	stack->p3 = oldStack->p3;		// rightmost point doesn't change
	oldStack->state = BEZIER_DONE;	// We're done after this recursion
	stack->state = BEZIER_LEFT;		// Tell the child to start by going left
	goto loop;

doneRecursion:
	
	curve.AddItem(p[3]);
};

#if 0
/*	This is exactly the same as the above, except that we stop the
	recursion if no part of the sub-bezier we are considering will
	be within the bounding rectangle. */
void TesselateClippedCubicBezier(
		fpoint *p, BArray<fpoint> &curve,
		frect clip, float maxError)
{
	BezierStackFrame stackFrames[BEZIER_STACK_SIZE],*stack,*oldStack;
	fpoint tmp;
	frect bound;
	float maxError2 = maxError*maxError;
	int32 stackCount=0;

	stack = stackFrames;
	
	stack->p0 = p[0];
	stack->p1 = p[1];
	stack->p2 = p[2];
	stack->p3 = p[3];
	stack->state = BEZIER_LEFT;

loop:

	bound.left = bound.right = stack->p0.h;
	bound.top = bound.bottom = stack->p0.v;
	if (stack->p1.h < bound.left) bound.left = stack->p1.h;
	if (stack->p2.h < bound.left) bound.left = stack->p2.h;
	if (stack->p3.h < bound.left) bound.left = stack->p3.h;
	if (stack->p1.h > bound.right) bound.right = stack->p1.h;
	if (stack->p2.h > bound.right) bound.right = stack->p2.h;
	if (stack->p3.h > bound.right) bound.right = stack->p3.h;
	if (stack->p1.v < bound.top) bound.top = stack->p1.v;
	if (stack->p2.v < bound.top) bound.top = stack->p2.v;
	if (stack->p3.v < bound.top) bound.top = stack->p3.v;
	if (stack->p1.v > bound.bottom) bound.bottom = stack->p1.v;
	if (stack->p2.v > bound.bottom) bound.bottom = stack->p2.v;
	if (stack->p3.v > bound.bottom) bound.bottom = stack->p3.v;

	if (clip.left > bound.left) bound.left = clip.left;
	if (clip.top > bound.top) bound.top = clip.top;
	if (clip.right < bound.right) bound.right = clip.right;
	if (clip.bottom < bound.bottom) bound.bottom = clip.bottom;
	
	if ((stackCount > (BEZIER_STACK_SIZE-1)) ||
		Colinear(stack->p0,stack->p1,stack->p2,stack->p3,maxError2) ||
		(bound.left >= bound.right) ||
		(bound.top >= bound.bottom)) {
		stackCount--;
		stack--;
		goto unrecurse;
	};

	tmp				= MidPoint(stack->p1,stack->p2);
	stack->l1		= MidPoint(stack->p0,stack->p1);
	stack->r1 		= MidPoint(stack->p2,stack->p3);
	stack->l2		= MidPoint(stack->l1,tmp);
	stack->r2		= MidPoint(tmp,stack->r1);
	stack->center	= MidPoint(stack->l2,stack->r2);
	
	oldStack = stack++;
	stackCount++;
	stack->p0 = oldStack->p0;		// Leftmost point doesn't change
	stack->p1 = oldStack->l1;		// l1 and l2 define the middle two
	stack->p2 = oldStack->l2;		//    control points for the left half
	stack->p3 = oldStack->center;	// center is the rightmost point of left half
	oldStack->state = BEZIER_RIGHT;	// Remind ourselves to go right next
	stack->state = BEZIER_LEFT;		// Tell the child to start by going left
	goto loop;

unrecurse:
			
	if ((stackCount<0) || (stack->state==BEZIER_DONE)) {
		if (stackCount<=0)
			goto doneRecursion;
		stackCount--;
		stack--;
		goto unrecurse;
	};
	
	curve.AddItem(stack->center);

	oldStack = stack++;
	stackCount++;
	stack->p0 = oldStack->center;	// center is the leftmost point of right half
	stack->p1 = oldStack->r2;		// r1 and r2 define the middle two
	stack->p2 = oldStack->r1;		//    control points for the right half
	stack->p3 = oldStack->p3;		// rightmost point doesn't change
	oldStack->state = BEZIER_DONE;	// We're done after this recursion
	stack->state = BEZIER_LEFT;		// Tell the child to start by going left
	goto loop;

doneRecursion:
	
	curve.AddItem(p[3]);
};
#endif
