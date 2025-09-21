
#ifndef DECORPART_H
#define DECORPART_H

#include <SupportDefs.h>
#include "DecorAtom.h"
#include "DecorTypes.h"
#include "BArray.h"

class DecorThumb;

/**************************************************************/

#define EVENT_ERROR				1
#define EVENT_HANDLED			2
#define EVENT_TERMINAL			4

#define LIMITS_ERROR			1
#define LIMITS_NEED_LAYOUT		2

#define LAYOUT_ERROR			1
#define LAYOUT_NEED_REDRAW		2
#define LAYOUT_REGION_CHANGED	4

struct CollectRegionStruct {
	DecorState *		instance;
	region *			theRegion;
	region *			subRegion;
	region *			tmpRegion[2];
};

struct HandleEventStruct {
	DecorState *		state;
	DecorMessage *		event;
	point 				localPoint;
	rect 				bounds;
	DecorPart *			leaf;
	Bitmap *			mouseContainers;
	Bitmap *			newMouseContainers;
	DecorExecutor *		executor;
	DecorThumb *		thumb;
};

enum {
	NO_LIMIT = -2,			// Magic value to indicate a limit is not specified
	INFINITE_LIMIT = -1		// Magic value to let limit grow without bound
};

enum {
	MIN_LIMIT = 0,			// Index for minimum size limit
	PREF_LIMIT = 1,			// Index for preferred size limit
	MAX_LIMIT = 2,			// Index for maximum size limit
	
	_NUM_LIMIT = 3			// Number of limit metrics
};

struct LimitsStruct {
	int16 widths[_NUM_LIMIT];
	int16 heights[_NUM_LIMIT];
	
	inline void clear() {
		static const LimitsStruct empty = {
			{ NO_LIMIT, NO_LIMIT, NO_LIMIT },
			{ NO_LIMIT, NO_LIMIT, NO_LIMIT }
		};
		*this = empty;
	}
	
	void update(const LimitsStruct& o);
	void fillin(const LimitsStruct& o);
	
	inline LimitsStruct& operator=(const LimitsStruct& o)	{ memcpy(this, &o, sizeof(o)); return *this; }
	inline bool operator==(const LimitsStruct& o)			{ return memcmp(this, &o, sizeof(o)) == 0; }
	inline bool operator!=(const LimitsStruct& o)			{ return memcmp(this, &o, sizeof(o)) != 0; }
	inline bool operator<(const LimitsStruct& o)			{ return memcmp(this, &o, sizeof(o)) < 0; }
	inline bool operator<=(const LimitsStruct& o)			{ return memcmp(this, &o, sizeof(o)) <= 0; }
	inline bool operator>=(const LimitsStruct& o)			{ return memcmp(this, &o, sizeof(o)) >= 0; }
	inline bool operator>(const LimitsStruct& o)			{ return memcmp(this, &o, sizeof(o)) > 0; }
};

// Helper functions for implementing groups.
void apply_gravity(rect* used, const LimitsStruct& limits, gravity g);

class DecorTable;
class DecorPart : public DecorAtom
{
				DecorPart&		operator=(DecorPart&);
								DecorPart(DecorPart&);
	
	static const LimitsStruct	no_limits;
	
	protected:
				DecorAtom *		m_parent;
				DecorAtom *		m_behavior;
				char			m_name[64];
				float			m_weight;
				uint16			m_parentDeps;
				uint8			m_gravity;
				LimitsStruct	m_limits;

	public:
								DecorPart();
		virtual					~DecorPart();

#if defined(DECOR_CLIENT)
								DecorPart(DecorTable *table);
		virtual	bool			IsTypeOf(const std::type_info &t);
		virtual void			Flatten(DecorStream *f);
		virtual	void			AllocateLocal(DecorDef *decor);
		virtual	void			RegisterAtomTree();
		virtual void			RegisterDependency(DecorAtom *dependant, uint32 depFlags);
#endif
		virtual	void			Unflatten(DecorStream *f);

		inline	const char *	Name() const { return m_name; }
		inline	float			Weight() const { return m_weight; }
		inline	gravity			Gravity() const { return (gravity)m_gravity; }
		inline	const LimitsStruct&	Limits() const { return m_limits; }
		
		inline	void			SetLimits(const LimitsStruct& lim) { m_limits = lim; }
		inline	void			UpdateLimits(const LimitsStruct& lim) { m_limits.update(lim); }
		inline	void			FillInLimits(const LimitsStruct& lim) { m_limits.fillin(lim); }
		
		virtual	visual_style	VisualStyle(DecorState *instance);
		
		virtual	void			CollectRegion(CollectRegionStruct *collect, rect bounds) {};
		virtual	uint32			Layout(	CollectRegionStruct *collect,
										rect newBounds, rect oldBounds);
		virtual	uint32			CacheLimits(DecorState *changes) { return 0; };
		virtual	void			FetchLimits(DecorState *changes,
											LimitsStruct *limits) = 0;
		virtual	void			Draw(		DecorState *instance, rect bounds,
											DrawingContext context, region *updateRegion) {};

		virtual void			PropogateChanges(
									const VarSpace *space,
									DecorAtom *instigator,
									DecorState *changes,
									uint32 depFlags,
									DecorAtom *newChoice=NULL);

		virtual	int32			HandleEvent(HandleEventStruct *handle);
};

/**************************************************************/

#endif
