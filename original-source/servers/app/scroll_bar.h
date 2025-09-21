//******************************************************************************
//
//	File: scroll_bar.h
//
//	Description: TScrollBar header.
//	
//	Written by:	Mathias Agopian
//
//	Copyright 2001, Be Incorporated
//
//******************************************************************************

#ifndef	SCROLL_BAR_H
#define	SCROLL_BAR_H

#include "view.h"
#include "DecorExecutor.h"
#include "DecorPart.h"

class MWindow;
class DecorState;
class DecorThumb;
class DecorEvent;

class TScrollBar : public TView , public DecorExecutor
{	
public:
			TScrollBar(rect*, int32 orientation, unsigned long = NO_CHANGE);
	virtual	~TScrollBar();

	virtual void	HandleEvent(BParcel *event);
	virtual void 	HandleDecorEvent(Event *);
	virtual	void	Action(DecorState *changes, BMessage *action);
	virtual	void    Draw(bool isUpdate);

	virtual	bool	ResetDecor();
		
	inline	void    get_min_max(int32 *min, int32 *max)  const	{ *min = f_min; *max = f_max; }
	inline	bool	vertical() const							{ return f_vertical; }
	
			void    set_min_max(int32 min, int32 max);
			void    set_value(int32, bool msg);
			void	set_proportion(float p);
			void	set_range(int32 min, int32 max);
			void	set_steps(int32 small_step, int32 big_step);

private:
	friend	class	MWindow;

	inline bool IsActive() const { return (Owner() && (Owner()->looks_active()) && (f_min < f_max)); }
	void send_value();
	void OpenTransaction();
	void OpenDecorTransaction();
	void CloseDecorTransaction();
	void Action_DragThumb();
	void Action_SetEvent(DecorEvent *event, uint32 time);
	void Action_SetAlarm(bigtime_t time);
	int32 Proportion() const;

	DecorState	*m_decor;
	DecorState	*m_newDecorState;
	DecorState	*m_mouseDownDecorInstance;
	Bitmap		m_mouseContainer1;
	Bitmap		m_mouseContainer2;
	Bitmap 		*m_lastMouseContainment;
	BParcel		*m_workingEvent;
	DecorState	*m_inputChangeRec;
	DecorPart	*m_mouseDownPart;
	bool		m_transactionOpen;
	int32		m_transaction_level;
	rect		m_oldBounds;
	bool		m_oldActive;

	// Dragging stuff
	enum {
		msNothing = 0,
		msDragThumb = 1
	};
	DecorThumb			*m_workingThumb;
	HandleEventStruct 	*m_workingEventStruct;
	point				m_dragPt;
	int32				m_mouseState;
	bigtime_t			m_lastMove;
	int32				m_lastDelta;

	// Scrollbar attributes
	float	f_proportion;
	int32	f_value;
	int32	s_value;
	int32	f_min;
	int32	f_max;
	int32	f_page_value;
	int32	f_inc_value;
	bool	f_vertical;
	
	// messaging
	uint32	m_queueIterValue;
	uint32	m_valueMsgToken;
};

#endif
