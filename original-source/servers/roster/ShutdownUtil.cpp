/*
 * File: ShutdownUtil.cpp
 *
 */ 

#include <Debug.h>

#include "main.h"
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fs_info.h>
#include <Alert.h>
#include <Entry.h>
#include <Path.h>
#include <Directory.h>
#include <signal.h>

#ifndef _SCREEN_H
#include <Screen.h>
#endif
#ifndef _BITMAP_H
#include <Bitmap.h>
#endif

#include <Roster.h>
#include <ShutdownUtil.h>

/* The list serves two purposes.. 
 *     1. the apps to skip in the first pass
 *     2. then the order in which to terminate the apps
 *        (the order is reverse of the list)
 */

char	*signatures[] = {
	// LAST APP TO BE TERMINATED GOES HERE
	"application/x-vnd.Be-input_server",	/* Input Server */
	"application/x-vnd.Be-DBSV",			/* Debug Server */
	"application/x-vnd.Be-SYSL",			/* SysLog Server */
	"application/x-vnd.Be.addon-host",	/* Media Add-On Server */
	"application/x-vnd.Be.media-server",	/* Media Server - before addon-host ! */
	"application/x-vnd.Be-AUSV",			/* Audio Server */
	"application/x-vnd.Be-NETS",			/* Network Server */
	"application/x-vnd.Be-POST",			/* Mail Server */
	"application/x-vnd.Be-PSRV",			/* Print Server */
	"application/x-vnd.Be-TRAK",			/* Tracker */
	"application/x-vnd.Be-TSKB"				/* Deskbar */
	// FIRST APP TO BE TERMINATED GOES HERE
};

int	signatureCount = sizeof(signatures)/sizeof(signatures[0]);

int
FindTeam(char *name)
{
	int	i = 0;

	for (i = 0; i < signatureCount; i++)
		if (strcasecmp(signatures[i], name) == 0)
			return i+2;

	return 0;
}

int
SortTeams(const void *_left, const void *_right)
{
	TeamVictim *left = *(TeamVictim **) _left, *right = *(TeamVictim **)_right;
	int			leftindex, rightindex;

	/*
	 * Okay, there's magic here..
	 *
 	 * Here's how the list is to be sorted for killing
	 *
 	 * Normal Apps		 (sort value 0)
	 * Background Apps	 (sort value 1)
	 * Critical Servers	 (sort value 2..n - may include background apps)
	 */

	leftindex = FindTeam(left->signature);
	rightindex = FindTeam(right->signature);

	if (leftindex == 0 && left->flags & B_BACKGROUND_APP)
		leftindex = 1;
	
	if (rightindex == 0 && right->flags & B_BACKGROUND_APP)
		rightindex = 1;

	if (leftindex < rightindex)
		return 1;
	else if (leftindex == rightindex)
		return 0;
	else
		return -1;
}

void
BuildAppList(BList &teamList, BList &skipList, bool orderKill,
	bool registered_only)
{
	int			i, j;
	status_t	err;
	team_id		team;
	bool		skip;
	TeamVictim	*victim;
	app_info	ainfo;
	BList		roosterList;
	BList		orderList;

	teamList.RemoveItems(0, teamList.CountItems());

	if (registered_only) {
		RealRoster->GetAppList(&roosterList);

		for (i = 0; i < roosterList.CountItems(); i++) {
			team = (team_id) roosterList.ItemAt(i);

			if (skipList.IndexOf((void*)team) >= 0)
				continue;

			err = RealRoster->GetRunningAppInfo(team, &ainfo);

			if (err != B_NO_ERROR)
				continue;

			skip = FALSE;

			if (strcasecmp(ainfo.signature, ROSTER_MIME_SIG) == 0 ||
				strcasecmp(ainfo.signature, KERNEL_MIME_SIG) == 0 ){
				continue;
			}

			for (j = 0; j < signatureCount; j ++) {
				if ((strcasecmp(signatures[j], ainfo.signature) == 0)
				|| (ainfo.flags & B_BACKGROUND_APP)) {
					if (orderKill) {
						victim = new TeamVictim;
						victim->team = team;
						victim->flags = ainfo.flags;
						strcpy(victim->signature, ainfo.signature);
						orderList.AddItem((void *) victim);
					}
					skip = TRUE;
					break;
				} 
			}
		
			if (!skip) 
				teamList.AddItem((void *) team);
		}

		if (orderKill) {
			orderList.SortItems(SortTeams);

			for (i = 0; i < orderList.CountItems(); i++) {
				victim = (TeamVictim *) orderList.ItemAt(i);
				teamList.AddItem((void *)victim->team);
				delete victim;
			}
		}
	} else {
		// Build a list of all the teams
		int32		cookie = 0;
		team_info	tinfo;
    	while (get_next_team_info(&cookie, &tinfo) == B_NO_ERROR) {
			if (!IsTeamNeeded(tinfo.args)) {
				if (skipList.IndexOf((void*)tinfo.team) >= 0)
					continue;
				teamList.AddItem((void*) tinfo.team);
			}
		}
	}
}

void
Basename(char *args, char *name)
{
	char	*ptr, *slash;
	int		i;

	ptr = args;
	slash = ptr;

	while (*ptr != ' ' && *ptr != '\0') {
		if (*ptr == '/')
			slash = ptr+1;
		ptr++;
	}

	i = ptr - slash;

	strncpy(name, slash, i);
	name[i] = '\0';
}

bool
IsTeamNeeded(char *args)
{
	char	name[128];

	Basename(args, name);

	return (strcmp("app_server", name) == 0)
		|| (strcmp("registrar", name) == 0) 
		|| (strcmp("kernel_team", name) == 0);
}

/*------------------------------------------------------------*/
/*------------------------------------------------------------*/
/*------------------------------------------------------------*/
static long hilite_table[256];
bool	hilite_table_init = false;

void MakeHiliteTable()
{
	rgb_color aColor;
	
	if (hilite_table_init)
		return;
	hilite_table_init = true;

	BScreen screen( B_MAIN_SCREEN_ID );

	for (long i = 0; i < 256; i++) {
		aColor = screen.ColorForIndex(i);

		aColor.red = (long)aColor.red * 2 / 3;
		aColor.green = (long)aColor.green * 2 / 3;
		aColor.blue = (long)aColor.blue * 2 / 3;

		hilite_table[i] = screen.IndexForColor(aColor);
	}

	hilite_table[B_TRANSPARENT_8_BIT] = B_TRANSPARENT_8_BIT;
}

/*------------------------------------------------------------*/

void MakeHilitedIcon(BBitmap *src, BBitmap *dest)
{
	long			mem_size;
	uchar*			bits;
	int				i;

	mem_size = dest->BitsLength();
	dest->SetBits(src->Bits(), mem_size, 0, B_COLOR_8_BIT);
	bits = (uchar*)dest->Bits();
	for (i = 0; i < mem_size; i++)
		bits[i] = hilite_table[bits[i]];
}


/*------------------------------------------------------------*/
/*------------------------------------------------------------*/
/*------------------------------------------------------------*/
