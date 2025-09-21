/********************* Revision Control Information **********************************
*                                                                                    *
*     $Header: //bliss/user/archive/rcs/true_type/fnt.h,v 14.0 97/03/17 17:53:28 shawn Release $                                                                       *
*                                                                                    *
*     $Log:	fnt.h,v $
 * Revision 14.0  97/03/17  17:53:28  shawn
 * TDIS Version 6.00
 * 
 * Revision 10.2  96/07/02  13:47:50  shawn
 * Changed boolean to btsbool.
 * 
 * Revision 10.1  95/04/11  12:23:52  roberte
 * Moved some fnt.c local macros to that file.
 * 
 * Revision 10.0  95/02/15  16:22:51  roberte
 * Release
 * 
 * Revision 9.2  95/01/11  13:26:15  shawn
 * Changed IFntFunc typedef to FntFunc
 * Defined typedefs for GetCVTEntry, GetSingleWidth, TraceFuncType and ScaleFuncType
 * Changed TraceFunc parameters to type TraceFuncType
 * Changed traceFunc parameter in GLOBAL_PROTO for fnt_Execute() to type TraceFuncType
 * 
 * Revision 9.1  95/01/04  16:32:20  roberte
 * Release
 * 
 * Revision 8.1  95/01/03  13:24:10  shawn
 * Converted to ANSI 'C'
 * Modified for support by the K&R conversion utility
 * 
 * Revision 8.0  94/05/04  09:28:47  roberte
 * Release
 * 
 * Revision 7.0  94/04/08  11:57:06  roberte
 * Release
 * 
 * Revision 6.92  94/03/18  14:00:33  roberte
 * Got rid of nested comment.
 * 
 * Revision 6.91  93/08/30  14:50:33  roberte
 * Release
 * 
 * Revision 6.44  93/03/15  13:08:24  roberte
 * Release
 * 
 * Revision 6.3  92/11/19  15:45:51  roberte
 * Release
 * 
 * Revision 6.2  92/04/30  11:22:06  leeann
 * take out non-Ascii bytes
 * 
 * Revision 6.1  91/08/14  16:45:08  mark
 * Release
 * 
 * Revision 5.1  91/08/07  12:26:20  mark
 * Release
 * 
 * Revision 4.3  91/08/07  11:50:12  mark
 * remove rcsstatus string
 * 
 * Revision 4.2  91/08/07  11:39:15  mark
 * added RCS control strings
 * 
*************************************************************************************/

/*
	File:		fnt.h

	Contains:	xxx put contents here (or delete the whole line) xxx

	Written by:	xxx put name of writer here (or delete the whole line) xxx

	Copyright:	) 1987-1990 by Apple Computer, Inc., all rights reserved.

	Change History (most recent first):

		 <6>	 12/5/90	MR		Remove unneeded leftSideBearing[in,out] and advanceWidth[in,out]
									fields. [rb]
		 <5>	11/27/90	MR		Better typedefs for function pointers [rb]
		 <4>	11/16/90	MR		More debugging code [rb]
		 <3>	 11/5/90	MR		Change globalGS.ppemDot6 to globalGS.fpem. InstrPtrs and
									curvePtr are now all uint8*. [rb]
		 <2>	10/20/90	MR		Restore changes since project died.  Converting to 2.14 vectors,
									smart math routines. [rb]
		<0+>	10/19/90	MR		Restore changes since project died.  Converting to 2.14 vectors,
									smart math routines.
		<10>	 7/26/90	MR		rearrange local graphic state, remove unused parBlockPtr
		 <9>	 7/18/90	MR		change loop variable from long to short, and other Ansi-changes
		 <8>	 7/13/90	MR		Prototypes for function pointers
		 <5>	  6/4/90	MR		Remove MVT
		 <4>	  5/3/90	RB		replaced dropoutcontrol with scancontrolin and scancontrol out
									in global graphics state
		 <3>	 3/20/90	CL		fields for multiple preprograms fields for ppemDot6 and
									pointSizeDot6 changed SROUND to take D/2 as argument
		 <2>	 2/27/90	CL		Added DSPVTL[] instruction.  Dropoutcontrol scanconverter and
									SCANCTRL[] instruction
	   <3.1>	11/14/89	CEL		Fixed two small bugs/feature in RTHG, and RUTG. Added SROUND &
									S45ROUND.
	   <3.0>	 8/28/89	sjk		Cleanup and one transformation bugfix
	   <2.2>	 8/14/89	sjk		1 point contours now OK
	   <2.1>	  8/8/89	sjk		Improved encryption handling
	   <2.0>	  8/2/89	sjk		Just fixed EASE comment
	   <1.7>	  8/1/89	sjk		Added composites and encryption. Plus some enhancementsI
	   <1.6>	 6/13/89	SJK		Comment
	   <1.5>	  6/2/89	CEL		16.16 scaling of metrics, minimum recommended ppem, point size 0
									bug, correct transformed integralized ppem behavior, pretty much
									so
	   <1.4>	 5/26/89	CEL		EASE messed up on RcS comments
	  <%1.3>	 5/26/89	CEL		Integrated the new Font Scaler 1.0 into Spline Fonts

	To Do:
*/
/*	rwb 4/24/90 Replaced dropoutControl with scanControlIn and scanControlOut in
		global graphics state. 
		<3+>	 3/20/90	mrr		Added support for IDEFs.  Made funcDefs long aligned
									by storing int16 length instead of int32 end.
*/

#define fnt_pixelSize 0x40L
#define fnt_pixelShift 6

#define MAXBYTE_INSTRUCTIONS 256

#define VECTORTYPE					ShortFrac
#define ONEVECTOR					ONESHORTFRAC
#define VECTORMUL(value, component)	ShortFracMul(value, component)
#define VECTORDOT(a,b)				ShortFracDot(a,b)
#define VECTORMULDIV(a,b,c)			ShortMulDiv(a,b,c)
#define ONESIXTEENTHVECTOR			((ONEVECTOR) >> 4)

typedef struct VECTOR {
	VECTORTYPE x;
	VECTORTYPE y;
} VECTOR;

typedef struct {
	F26Dot6 *x;		/* The Points the Interpreter modifies */
    F26Dot6 *y;		/* The Points the Interpreter modifies */
    F26Dot6 *ox;	/* Old Points */
    F26Dot6 *oy;	/* Old Points */
    F26Dot6 *oox;	/* Old Unscaled Points, really ints */
    F26Dot6 *ooy;	/* Old Unscaled Points, really ints */
	uint8	*onCurve; /* indicates if a point is on or off the curve */
	int16 	 nc; 	/* Number of contours */
	int16	 padWord;	/* <4> */
	int16	*sp; 	/* Start points */
	int16	*ep; 	/* End points */
	uint8	*f; 	/* Internal flags, one byte for every point */
} fnt_ElementType;

typedef struct {
    int32 start;		/* offset to first instruction */
	uint16 length;		/* number of bytes to execute <4> */
	uint16 pgmIndex;	/* index to appropriate preprogram for this func (0..1) */
} fnt_funcDef;

/* <4> pretty much the same as fnt_funcDef, with the addition of opCode */
typedef struct {
	int32 start;
	uint16 length;
	uint8  pgmIndex;
	uint8  opCode;
} fnt_instrDef;

typedef struct {
	Fract x;
	Fract y;
} fnt_FractPoint;

/* Define the conditional parameter macros here */

struct fnt_LocalGraphicStateType;
struct fnt_GlobalGraphicStateType;

#if REENTRANT_ALLOC
#define GSP_DECL0 struct fnt_LocalGraphicStateType *pLocalGS
#define GSP_DECL1 GSP_DECL0,
#else
#define GSP_DECL0 void
#define GSP_DECL1
#endif

/* typedefs for callback functions */

typedef void	(*FntFunc)(GSP_DECL0);
typedef void	(*InterpreterFunc)(GSP_DECL1 uint8*, uint8*);
typedef void	(*FntMoveFunc)(GSP_DECL1 fnt_ElementType*, ArrayIndex, F26Dot6);
typedef F26Dot6	(*FntRoundFunc)(GSP_DECL1 F26Dot6, F26Dot6);
typedef F26Dot6	(*FntProjFunc)(GSP_DECL1 F26Dot6, F26Dot6);
typedef F26Dot6 (*GetCVTEFunc)(GSP_DECL1 ArrayIndex n);
typedef F26Dot6 (*GetSWidFunc)(GSP_DECL0);

/* typedefs for TraceFunc and ScaleFunc callbacks */

typedef void	(*TraceFuncType)(struct fnt_LocalGraphicStateType*);
typedef F26Dot6 (*ScaleFuncType)(struct fnt_GlobalGraphicStateType*, F26Dot6);

typedef struct {

/* PARAMETERS CHANGEABLE BY TT INSTRUCTIONS */

	F26Dot6 wTCI;     				/* width table cut in */
	F26Dot6 sWCI;     				/* single width cut in */
	F26Dot6 scaledSW; 				/* scaled single width */
	int32 scanControl;				/* controls kind and when of dropout control */
	int32 instructControl;			/* controls gridfitting and default setting */
	
	F26Dot6 minimumDistance;		/* moved from local gs  7/1/90  */
	FntRoundFunc RoundValue;		/*								*/
	F26Dot6	periodMask; 			/* ~(gs->period-1) 				*/
	Fract	period45;				/*								*/
	int16	period;					/* for power of 2 periods 		*/
	int16 	phase;					/*								*/
	int16 	threshold;				/* moved from local gs  7/1/90  */
	
	int16 deltaBase;
	int16 deltaShift;
	int16 angleWeight;
	int16 sW;         				/* single width, expressed in the same units as the character */
	int8 autoFlip;   				/* The auto flip Boolean */
	int8 pad;	

} fnt_ParameterBlock;				/* this is exported to client */

#define MAXANGLES		20
#define ROTATEDGLYPH	0x100
#define STRETCHEDGLYPH  0x200
#define NOGRIDFITFLAG	1
#define DEFAULTFLAG		2

typedef enum {
	PREPROGRAM,
	FONTPROGRAM,
	MAXPREPROGRAMS
} fnt_ProgramIndex;

typedef struct fnt_GlobalGraphicStateType {

	F26Dot6* stackBase; 			/* the stack area */
	F26Dot6* store; 				/* the storage area */
	F26Dot6* controlValueTable; 	/* the control value table */
	
	uint16	pixelsPerEm; 			/* number of pixels per em as an integer */
	uint16	pointSize; 				/* the requested point size as an integer */
	Fixed	fpem;					/* fractional pixels per em    <3> */
	F26Dot6 engine[4]; 				/* Engine Characteristics */
	
	fnt_ParameterBlock defaultParBlock;	/* variables settable by TT instructions */
	fnt_ParameterBlock localParBlock;

	/* Only the above is exported to Client throught FontScaler.h */

/* VARIABLES NOT DIRECTLY MANIPULABLE BY TT INSTRUCTIONS  */
	
    FntFunc* function; 				/* pointer to instruction definition area */
	fnt_funcDef* funcDef; 			/* function Definitions identifiers */
	fnt_instrDef* instrDef;			/* instruction Definitions identifiers */
   
	ScaleFuncType ScaleFunc;		/* Call back function to do scaling */
	
	uint8* pgmList[MAXPREPROGRAMS];	/* each program ptr is in here */
	
    /* These are parameters used by the call back function */

	Fixed  fixedScale; 			/* fixed sc aling factor */
	int32  nScale; 				/* numerator required to scale points to the right size*/
	int32  dScale; 				/* denumerator required to scale points to the right size */
	int16  shift; 				/* 2log of dScale */
	int8   identityTransformation; 	/* true/false  (does not mean identity from a global sense) */
	int8   non90DegreeTransformation; /* bit 0 is 1 if non-90 degree, bit 1 is 1 if x scale doesn't equal y scale */
	Fixed  xStretch; 			/* Tweaking for glyphs under transformational stress <4> */
	Fixed  yStretch;
	
	/** these two together make fnt_AngleInfo **/

	fnt_FractPoint*	anglePoint;
	int16*			angleDistance;

    int8 init; 						/* executing preprogram ?? */
	uint8 pgmIndex;					/* which preprogram is current */
	LoopCount instrDefCount;		/* number of currently defined IDefs */

#ifdef DEBUG
	sfnt_maxProfileTable*	maxp;
	uint16					cvtCount;
	uint16					glyphIndex;
	btsbool					glyphProgram;
#endif

} fnt_GlobalGraphicStateType;


/* 
 * This is the local graphics state  
 */

typedef struct fnt_LocalGraphicStateType {
	
	fnt_ElementType *CE0, *CE1, *CE2; 	/* The character element pointers */
	VECTOR proj; 						/* Projection Vector */
	VECTOR free;						/* Freedom Vector */
	VECTOR oldProj; 					/* Old Projection Vector */
	F26Dot6 *stackPointer;

	uint8 *insPtr; 						/* Pointer to the instruction we are about to execute */
    fnt_ElementType *elements;
    fnt_GlobalGraphicStateType *globalGS;

	TraceFuncType TraceFunc;			/* Call back to do tracing */

    ArrayIndex Pt0, Pt1, Pt2; 			/* The internal reference points */
	int16   roundToGrid;			
	LoopCount loop; 					/* The loop variable */	
	uint8 opCode; 						/* The instruction we are executing */
	uint8 padByte;
	int16 padWord;

	/* Above is exported to client in FontScaler.h */

	VECTORTYPE pfProj; /* = pvx * fvx + pvy * fvy */

	FntMoveFunc MovePoint;
	FntProjFunc Project;
	FntProjFunc OldProject;
	InterpreterFunc Interpreter;
    GetCVTEFunc GetCVTEntry;
    GetSWidFunc GetSingleWidth;

	jmp_buf	env;		/* always be at the end, since it is unknown size */

} fnt_LocalGraphicStateType;

/*
 * Executes the font instructions.
 * This is the external interface to the interpreter.
 *
 * Parameter Description
 *
 * elements points to the character elements. Element 0 is always
 * reserved and not used by the actual character.
 *
 * ptr points at the first instruction.
 * eptr points to right after the last instruction
 *
 * globalGS points at the global graphics state
 *
 * TraceFunc is pointer to a callback functioned called with a pointer to the
 *		local graphics state if TraceFunc is not null. The call is made just before
 *		every instruction is executed.
 *
 * Note: The stuff globalGS is pointing at must remain intact
 *       between calls to this function.
 */

GLOBAL_PROTO int fnt_Execute(fnt_ElementType *elements, uint8 *ptr, uint8 *eptr,
                             fnt_GlobalGraphicStateType *globalGS, fs_FuncType TraceFunc);

/*
 * Init routine, to be called at boot time.
 */

GLOBAL_PROTO void fnt_Init(fnt_GlobalGraphicStateType *globalGS);

