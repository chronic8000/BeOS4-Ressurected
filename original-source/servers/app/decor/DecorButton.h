
#ifndef DECORBUTTON_H
#define DECORBUTTON_H

#include "DecorWidget.h"

/**************************************************************/

class DecorButton : public DecorWidget
{
		DecorAtom *				m_buttonImages[2];
		DecorAtom *				m_actionDef;
		uint8					m_flags;
		int32					m_localVars;
		rect					m_hotRect;

		DecorButton&			operator=(DecorButton&);
								DecorButton(DecorButton&);
		DecorAtom *				WhichImage(uint8 flags);

	public:
								DecorButton();
		virtual					~DecorButton();
		
#if defined(DECOR_CLIENT)
								DecorButton(DecorTable *table);
		virtual	bool			IsTypeOf(const std::type_info &t);
		virtual	void			AllocateLocal(DecorDef *decor);
		virtual	void			RegisterAtomTree();
		virtual	DecorAtom *		Reduce(int32 *proceed);
		virtual void			Flatten(DecorStream *f);
#endif
		virtual	void			Unflatten(DecorStream *f);

		virtual	void			InitLocal(DecorState *instance);

		virtual	void			GetImage(DecorState *instance, DecorImage **image);

				uint32			MouseMoved(HandleEventStruct *handle);
				uint32			MouseDown(HandleEventStruct *handle);
				uint32			MouseUp(HandleEventStruct *handle);
		
		virtual	int32			HandleEvent(HandleEventStruct *handle);

		#define THISCLASS		DecorButton
		#include				"DecorAtomInclude.h"
};

#endif
