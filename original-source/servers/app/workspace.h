/*******************************************************************************
//
//	File:		workspace.h
//
//	Description:	workspace headers
//	
//	Written by:	Robert Polic
//
//	Copyright 1996, Be Incorporated
//
  *****************************************************************************/

#ifndef	WORKSPACE_H
#define	WORKSPACE_H

class	TWindow;
class	TWSView;

/*===============================================================*/

class	TWS {
public:
TWindow*			fWindow;
TWSView*			fView;

					TWS(TWindow*);
			void	DeskRGB(int32 index);
			void	SetWorkspace(long);
			void	WindowChanged(TWindow*);
			void	Refresh();
};
#endif



