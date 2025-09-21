#include "MethodReplicant.h"
#include "InputServer.h"
#include "AddOns.h"

#include <malloc.h>
#include <string.h>

#include <AppDefs.h>
#include <Bitmap.h>
#include <Debug.h>
#include <PopUpMenu.h>
#include <Window.h>


const BRect kBitmapRect(0.0, 0.0, 15.0, 15.0);
const float	kIconToNameSeparator 		= 6.0;
const float kSubmenuIndicatorSeparator	= 10.0;


MethodMenuItem::MethodMenuItem(
	int32		cookie,
	const char	*name,
	const uchar	*icon)
		: BMenuItem(name, new BMessage(msg_SetMethod))
{
	fCookie = cookie;
	fName = NULL;
	fIcon = NULL;
	fMenu = NULL;
	fHeightOffset = 0.0;

	SetName(name);
	SetIcon(icon);
}


MethodMenuItem::MethodMenuItem(
	int32		cookie,
	const char	*name,
	const uchar	*icon,
	BMenu		*menu,
	BMessenger	target)
		: BMenuItem(menu, new BMessage(msg_SetMethod))
{
	fCookie = cookie;
	fName = NULL;
	fIcon = NULL;
	fMenu = menu;
	fTarget = target;
	fHeightOffset = 0.0;

	SetName(name);
	SetIcon(icon);
}


MethodMenuItem::~MethodMenuItem()
{
	free(fName);
	delete (fIcon);
	// fMenu is owned by superclass (BMenuItem)
}


void
MethodMenuItem::SetName(
	const char	*name)
{
	free(fName);
	
	fName = strdup(name);
	SetLabel(fName);
}


const char*
MethodMenuItem::Name() const
{
	return (fName);
}


void
MethodMenuItem::SetIcon(
	const uchar	*icon)
{
	delete (fIcon);

	fIcon = new BBitmap(kBitmapRect, B_COLOR_8_BIT);
	fIcon->SetBits(icon, fIcon->BitsLength(), 0, B_COLOR_8_BIT);
}


const BBitmap*
MethodMenuItem::Icon() const
{
	return (fIcon);
}


int32
MethodMenuItem::Cookie() const
{
	return (fCookie);
}


BMessenger
MethodMenuItem::Target() const
{
	return (fTarget);
}


void
MethodMenuItem::GetContentSize(
	float	*width,
	float	*height)
{
	*width = ceil(kBitmapRect.Width() + kIconToNameSeparator + be_plain_font->StringWidth(Label()));
	if (fMenu != NULL)
		*width += kSubmenuIndicatorSeparator;

	*height = ceil(kBitmapRect.Height());
	
	font_height fontHeight;
	be_plain_font->GetHeight(&fontHeight);

	float theHeight = ceil(fontHeight.ascent + fontHeight.descent + fontHeight.leading);
	*height = (theHeight > *height) ? theHeight : *height;

	fHeightOffset = *height - ceil(fontHeight.ascent + fontHeight.descent + fontHeight.leading);
	fHeightOffset /= 2;
}


void
MethodMenuItem::DrawContent()
{
	BView	*view = Menu();
	BPoint	loc = ContentLocation();
	loc.y += ((Frame().Height() - kBitmapRect.Height()) / 2) - 1.0;

	view->SetDrawingMode(B_OP_OVER);
	view->DrawBitmapAsync(fIcon, loc);

	view->MovePenBy(ceil(kBitmapRect.Width() + kIconToNameSeparator), fHeightOffset);

	BMenuItem::DrawContent();
}


MethodReplicant::MethodReplicant(
	BMessage	*message)
		: BView(message)
{
	fMethodMenu = new BPopUpMenu(B_EMPTY_STRING);
	fMethodMenu->SetFont(be_plain_font);
	fMethodMenu->SetRadioMode(true);
	fCurMethodItem = NULL;

	int32		numMethods = 0;
	type_code	theType = 0;
	message->GetInfo("name", &theType, &numMethods);

	for (int32 i = 0; i < numMethods; i++) {
		int32 cookie = message->FindInt32("cookie", i);

		const char *name = message->FindString("name", i);

		const uchar	*icon = NULL;
		ssize_t		iconSize = 0;
		message->FindData("icon", B_RAW_TYPE, i, (const void **)&icon, &iconSize);

		BMessage archivedMenu;
		message->FindMessage("menu", i, &archivedMenu);
		BMenu *menu = dynamic_cast<BMenu *>(instantiate_object(&archivedMenu));

		if (menu == NULL)
			fMethodMenu->AddItem(new MethodMenuItem(cookie, name, icon));
		else {
			BMessenger target;
			message->FindMessenger("target", i, &target);

			fMethodMenu->AddItem(new MethodMenuItem(cookie, name, icon, menu, target));
		}
	}

	fCurMethodItem = (MethodMenuItem *)fMethodMenu->ItemAt(0);
	fCurMethodItem->SetMarked(true);
}


MethodReplicant::~MethodReplicant()
{
	delete (fMethodMenu);
}


status_t
MethodReplicant::ArchiveFactory(
	BMessage	*message,
	BList		*methodList)
{
	int32 numMethods = methodList->CountItems();
	if (numMethods < 1)
		return (B_ERROR);

	BView view(kBitmapRect, B_EMPTY_STRING, B_FOLLOW_ALL, B_WILL_DRAW);
	status_t err = view.Archive(message);
	if (err != B_NO_ERROR)
		return (err);

	message->ReplaceString("class", "MethodReplicant");
	message->AddString("add_on", kInputServerSignature);

	for (int32 i = 0; i < numMethods; i++) {
		_BMethodAddOn_ *method = (_BMethodAddOn_ *)methodList->ItemAt(i);

		message->AddInt32("cookie", (int32)method);
		message->AddString("name", method->Name());
		message->AddData("icon", B_RAW_TYPE, method->Icon(), method->IconSize());
		message->AddMessage("menu", method->MenuArchive());	
		message->AddMessenger("target", method->MenuTarget());
	}	

	return (B_NO_ERROR);
}


BArchivable*
MethodReplicant::Instantiate(
	BMessage	*data)
{
	return (new MethodReplicant(data));
}


void
MethodReplicant::AttachedToWindow()
{
	SendToInputServer(NULL);

	SetViewColor(Parent()->ViewColor());
	SetDrawingMode(B_OP_OVER);
}


void
MethodReplicant::MessageReceived(
	BMessage	*message)
{
	switch (message->what) {
		case msg_UpdateMethod:
			UpdateMethod(message);
			break;

		case msg_UpdateMethodName:
			UpdateMethodName(message);
			break;

		case msg_UpdateMethodIcon:
			UpdateMethodIcon(message);
			break;

		case msg_UpdateMethodMenu:
			UpdateMethodMenu(message);
			break;

		default:
			BView::MessageReceived(message);
			break;
	}
}


void
MethodReplicant::Draw(
	BRect)
{
	DrawBitmap(fCurMethodItem->Icon());
}


void
MethodReplicant::MouseDown(
	BPoint	where)
{
	BMenuItem *item = fMethodMenu->Go(ConvertToScreen(where));

	if (item != NULL) {
		if (fMethodMenu->IndexOf(item) < 0) {
			BMenuItem *superItem = item->Menu()->Superitem();
			do {
				MethodMenuItem *methodItem = dynamic_cast<MethodMenuItem *>(superItem);

				if (methodItem != NULL) {
					methodItem->Target().SendMessage(item->Message());
					break;
				}

				superItem = superItem->Menu()->Superitem();
			} while (superItem != NULL);
		}
		else {
			MethodMenuItem *methodItem = (MethodMenuItem *)item;

			if (methodItem != fCurMethodItem) {
				BMessage theCommand(msg_SetMethod);
				theCommand.AddInt32("cookie", methodItem->Cookie());
		
				SendToInputServer(&theCommand);
			}
		}
	}
}


MethodMenuItem*
MethodReplicant::FindItemByCookie(
	int32	cookie) 
{
	int32 numItems = fMethodMenu->CountItems();

	for (int32 i = 0; i < numItems; i++) {
		MethodMenuItem *theItem = (MethodMenuItem *)fMethodMenu->ItemAt(i);

		if (theItem->Cookie() == cookie)
			return (theItem);
	}

	return (NULL);
}


void
MethodReplicant::UpdateMethod(
	BMessage	*message)
{
	MethodMenuItem *item = FindItemByCookie(message->FindInt32("cookie"));

	if (item == NULL)
		return;

	if (item != fCurMethodItem) {
		fCurMethodItem = item;
		fCurMethodItem->SetMarked(true);

		Invalidate();
		Window()->UpdateIfNeeded();
	}
}


void
MethodReplicant::UpdateMethodName(
	BMessage	*message)
{
	MethodMenuItem *item = FindItemByCookie(message->FindInt32("cookie"));
	
	if (item == NULL)
		return;

	item->SetName(message->FindString("name"));
}


void
MethodReplicant::UpdateMethodIcon(
	BMessage	*message)
{
	MethodMenuItem *item = FindItemByCookie(message->FindInt32("cookie"));
	
	if (item == NULL)
		return;

	const uchar	*icon = NULL;
	ssize_t		iconSize = 0;
	message->FindData("icon", B_RAW_TYPE, (const void **)&icon, &iconSize);

	item->SetIcon(icon);

	if (fCurMethodItem == item) {
		Invalidate();
		Window()->UpdateIfNeeded();
	}
}


void
MethodReplicant::UpdateMethodMenu(
	BMessage	*message)
{
	MethodMenuItem *item = FindItemByCookie(message->FindInt32("cookie"));
	
	if (item == NULL)
		return;

	int32 itemIndex = fMethodMenu->IndexOf(item);
	fMethodMenu->RemoveItem(itemIndex);

	BMessage archivedMenu;
	message->FindMessage("menu", &archivedMenu);
	BMenu 			*menu = dynamic_cast<BMenu *>(instantiate_object(&archivedMenu));
	MethodMenuItem	*newItem = NULL;

	if (menu == NULL)
		newItem = new MethodMenuItem(item->Cookie(), item->Name(), (uchar *)item->Icon()->Bits());
	else {
		BMessenger target;
		message->FindMessenger("target", &target);

		newItem = new MethodMenuItem(item->Cookie(), item->Name(), (uchar *)item->Icon()->Bits(), menu, target);
	}

	fMethodMenu->AddItem(newItem, itemIndex);

	if (fCurMethodItem == item) {
		fCurMethodItem = newItem;
		fCurMethodItem->SetMarked(true);
	}

	delete (item);
}


void
MethodReplicant::SendToInputServer(
	BMessage	*message)
{
	if (!fInputServer.IsValid()) {
		fInputServer = BMessenger(kInputServerSignature);

		BMessage address(msg_MethodReplicantAddress);
		address.AddMessenger("address", BMessenger(this));

		fInputServer.SendMessage(&address);
	}

	if (message != NULL)
		fInputServer.SendMessage(message);			
}

