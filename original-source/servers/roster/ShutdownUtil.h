/*
 * File: ShutdownUtil.h
 */ 

#ifndef SHUTDOWN_UTIL_H
#define SHUTDOWN_UTIL_H

#ifndef MAIN_H
#include <main.h>
#endif

int FindTeam(char *name);
int SortTeams(const void *_left, const void *_right);
void Basename(char *args, char *name);
bool IsTeamNeeded(char *args);
void UnmountAllVolumes(void);
void BuildAppList(BList &teams, BList &skip, bool order, bool reg_only = true);
void MakeHilitedIcon(BBitmap *src, BBitmap *dest);
void MakeHiliteTable();

extern char *signatures[];
extern _TRoster_	*RealRoster;

struct TeamVictim {
	char	signature[B_MIME_TYPE_LENGTH];
	uint32	flags;
	team_id	team;
};

extern const char *ROSTER_MIME_SIG;
extern const char *KERNEL_MIME_SIG;

#define	ONE_SECOND		(1000000)
#define	QUIT_TIMEOUT	(10*ONE_SECOND)
#define	SIGHUP_TIMEOUT	(3*ONE_SECOND)

#endif
