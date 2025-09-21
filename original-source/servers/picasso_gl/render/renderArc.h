
#ifndef NEWARC_H
#define NEWARC_H

struct RenderContext;

enum arc_sides {
	ARC_LEFT=1,
	ARC_RIGHT=2
};

struct arc {
	arc *next;
	uint8 sides;
	int8 dir;
	int16 yTop,yBot;
	float c1,c3;
	float cx,cy;
};

enum arc_rule {
	B_ARC_NO_CONNECT=0,
	B_ARC_CONNECT_BEGIN_END=-1,
	B_ARC_CONNECT_END_BEGIN=1
};

extern void SetupArc(				fpoint center, fpoint radius, 
									float startAng, float stopAng, 
									arc &a, arc &b, arc &c, int32 &arcSections,
									fpoint &start, fpoint &stop,
									bool forFill=false,
									fpoint *last=NULL, fpoint *next=NULL);

extern void StrokeThinRoundRect(	RenderContext *context,
									frect frame, fpoint radius);
extern void StrokeThickRoundRect(	RenderContext *context,
									frect frame, fpoint radius);
extern void FillRoundRect(			RenderContext *context,
									frect frame, fpoint radius);

extern void StrokeThinArc(			RenderContext *context, 
									fpoint center, fpoint radius, 
									float arcBegin, float arcLen);
extern void StrokeThickArc(			RenderContext *context, 
									fpoint center, fpoint radius, 
									float arcBegin, float arcLen);
extern void FillArc(				RenderContext *context, 
									fpoint center, fpoint radius, 
									float arcBegin, float arcLen);

extern void StrokeThinEllipse(		RenderContext *context, 
									fpoint center, fpoint radius);
extern void StrokeThickEllipse(		RenderContext *context, 
									fpoint center, fpoint radius,
									float penWidth);
extern void FillEllipse(			RenderContext *context,
									fpoint center, fpoint radius);

#endif
