#ifndef _METHODREPLICANT_H
#define _METHODREPLICANT_H

#include <MenuItem.h>
#include <View.h>


const uint32 msg_MethodReplicantAddress	= 'MRmr';
const uint32 msg_SetMethod				= 'MRsm';
const uint32 msg_UpdateMethod			= 'MRu!';
const uint32 msg_UpdateMethodName		= 'MRun';
const uint32 msg_UpdateMethodIcon		= 'MRui';
const uint32 msg_UpdateMethodMenu		= 'MRum';


class MethodMenuItem : public BMenuItem {
public:
						MethodMenuItem(int32 		cookie,
									   const char	*name,
									   const uchar	*icon);
						MethodMenuItem(int32		cookie,
									   const char	*name, 
									   const uchar	*icon, 
									   BMenu		*menu,
									   BMessenger	target);
	virtual				~MethodMenuItem();

	void				SetName(const char *name);
	const char*			Name() const;

	void				SetIcon(const uchar *icon);
	const BBitmap*		Icon() const;

	int32				Cookie() const;
	BMessenger			Target() const;

protected:
	virtual void		GetContentSize(float *width, float *height);
	virtual void		DrawContent();

private:
	int32				fCookie;
	char*				fName;
	BBitmap*			fIcon;
	BMenu*				fMenu;
	BMessenger			fTarget;
	float				fHeightOffset;
};


// everything that is in this class needs to be
// explicitly exported from input_server.exp!!!

class MethodReplicant : public BView {
public:
						MethodReplicant(BMessage *message);
						~MethodReplicant();

	static status_t		ArchiveFactory(BMessage *message, BList *methodList);
	static BArchivable*	Instantiate(BMessage *data);

	virtual void		AttachedToWindow();
	virtual void		MessageReceived(BMessage *message);
	virtual void		Draw(BRect updateRect);
	virtual void		MouseDown(BPoint where);

private:
	MethodMenuItem*		FindItemByCookie(int32 cookie);

	void				UpdateMethod(BMessage *message);
	void				UpdateMethodName(BMessage *message);
	void				UpdateMethodIcon(BMessage *message);
	void				UpdateMethodMenu(BMessage *message);

	void				SendToInputServer(BMessage *message);

private:
	BMessenger			fInputServer;
	BPopUpMenu*			fMethodMenu;
	MethodMenuItem*		fCurMethodItem;
};


#endif
