#include <stdio.h>

#include <Application.h>

#if (_PRODUCT_IAD == 1)
#	include <www/TellBrowser.h>
#	include <Alert.h>
#	include <netconfig.h>
#	include <nsmessages.h>
#endif

#include "sc_daemon.h"
#include "sc_debug.h"


#define DEBUG 1

#if DEBUG > 0
#	define D(_x)	_x
#	define bug		print_error
#else
#	define D(_x)	;
#	define bug
#endif


#define CSC0_VALUE	0x31415926
#define CSC1_VALUE	0x27182818
#define CSC2_VALUE	0x14142135

// Name of the reader we're gonna use (hardcoded)
//const STR SCDApplication::kReaderName("Gemplus_GCR410_0");
const STR SCDApplication::kReaderName("Gemplus_GEMPC430_0");
static bool IsMachineSleeping();

// Our main function
int main(void)
{
	SCDApplication app;
	app.Run();
	return 0;
}

// -----------------------------------------------------------------------
// #pragma mark -


// Our smartcard daemon
SCDApplication::SCDApplication()
	: BApplication("application/x-vnd.Be-SCDM"),
	fResMgr(NULL),
	fScardTrack(NULL),
	fScardComm(NULL),
	fQuitIndicator(false),
	fDaemonThreadID(-1),
	fCurrentAlert(ALERT_NO_ALERT)
{
}

SCDApplication::~SCDApplication()
{
	delete fScardTrack;
	delete fScardComm;

	if (fResMgr)
	{ // If we are connected, disconnect from the resource manager
		RESPONSECODE result = fResMgr->ReleaseContext();
		D(bug("RESOURCEMANAGER::ReleaseContext()", result));
		delete fResMgr;
	}
}

void SCDApplication::ReadyToRun(void)
{
	fResMgr = new RESOURCEMANAGER;

	RESPONSECODE result = fResMgr->EstablishContext(SCARD_SCOPE_SYSTEM, 0, 0);
	D(bug("RESOURCEMANAGER::EstablishContext()", result));
	if (result != SCARD_S_SUCCESS)
		PostMessage(B_QUIT_REQUESTED);

	fScardComm = new SCARDCOMM(fResMgr);
	fScardTrack = new SCARDTRACK(fResMgr);
	
	fReaders.MakeEmpty();
	fReaders.AddItem(kReaderName);

	// The daemon thread
	resume_thread(fDaemonThreadID = spawn_thread(_Thread, "sc_daemon", B_NORMAL_PRIORITY, this));
}

bool SCDApplication::QuitRequested(void)
{
	RESPONSECODE result;

	// Tell our daemon thread that we must quit
	fQuitIndicator = true;
	
	// Break the current card tracking
	result = fScardTrack->Cancel();
	D(bug("SCARDTRACK::Cancel()", result));

	// Break the current communication
	result = fScardComm->Cancel();
	D(bug("SCARDCOMM::Cancel()", result));

	// Wait the daemon thread
	status_t dummy;
	wait_for_thread(fDaemonThreadID, &dummy);

	// Ok to quit
	return true;
}

void SCDApplication::MessageReceived(BMessage *msg)
{
//	D(msg->PrintToStream();)
	switch (msg->what)
	{

#if (_PRODUCT_IAD == 1)
		case 'SCLP':
			if (*fDefaultLogPass.login)
			{
				BMessage sci(PPP_SMARTCARD_INFO);
				sci.AddString("username", fDefaultLogPass.login);
				sci.AddString("password", fDefaultLogPass.password);
				msg.SendReply(&sci);
			}
			break;
#endif

		case MSG_SCARD_STATUS_CHANGED:
			{
				int32 currentstate	= msg->FindInt32("currentstate");
				int32 eventstate	= msg->FindInt32("eventstate");
				
				if ((eventstate & SCARD_STATE_EMPTY) == SCARD_STATE_EMPTY)
				{ // Reader empty
					D(printf("MessageReceived() reader empty\n"));
				}
				else if ((eventstate & SCARD_STATE_PRESENT) == SCARD_STATE_PRESENT)
				{ // smartcard present
					D(printf("MessageReceived() Card present\n"));
					if ((msg->FindBool("atr") == true) && ((eventstate & SCARD_STATE_ATRMATCH) != SCARD_STATE_ATRMATCH))
					{ // ATR didn't matched - unknown card
						D(printf("MessageReceived() ATR didn't matched!\n"));
						alert(ALERT_CARD_UNKNOWN);
					}
				}
				else if (((currentstate & SCARD_STATE_PRESENT) == SCARD_STATE_PRESENT) &&
						((eventstate & SCARD_STATE_UNAVAILABLE) == SCARD_STATE_UNAVAILABLE))
				{ // The card was present, and now the state is unavaillable : the card is inserted the wrong way!
					D(printf("MessageReceived() wrong way!\n"));
					alert(ALERT_CARD_WRONG_WAY);
				}
			}
			break;

		case MSG_SCARD_USER_UNKNOWN:
			alert(ALERT_CARD_UNKNOWN);
			break;

#if (_PRODUCT_IAD == 1)
		case TB_CMD_REPLY:
			// An alert has been closed
			fCurrentAlert = ALERT_NO_ALERT;
			break;
#endif

		default:
			BApplication::MessageReceived(msg);
	}
}



int32 SCDApplication::Thread(void)
{
	RESPONSECODE result;

	while (fQuitIndicator == false)
	{
		// Locate a known card in one of the connected reader of interest
		// In case of error, just retry or exist if asked to
		if (locate_card() != SCARD_S_SUCCESS)
			continue;

		// Here, we have a card in the reader that matched one of our ATR
		// We can now try do connect to that card
		if (connect_card() != SCARD_S_SUCCESS)
			continue; // This should not happen
			
		// Here we can do what we want with the card
		if (do_card() != SCARD_S_SUCCESS)
			continue;

		// Disconnect from the card
		disconnect_card();
					
		// Here me can wait the user removes the card
		wait_remove_card();
	}

	// Be sure to disconnect : Eject the card and shutdown
	result = fScardComm->Disconnect(SCARD_POWER_DOWN_CARD);
	D(bug("SCARDCOMM::Disconnect()", result));

	return result;
}


status_t SCDApplication::notify_application(SCARD_READERSTATE& r, bool atr)
{
	BMessage m(MSG_SCARD_STATUS_CHANGED);
	m.AddInt32("currentstate", r.CurrentState);
	m.AddInt32("eventstate", r.EventState);
	m.AddInt32("atr", atr);
	return PostMessage(&m);
}


RESPONSECODE SCDApplication::locate_card()
{
	RESPONSECODE result = SCARD_E_ERROR;
	STR_LIST cards;
	SCARD_READERSTATE_LIST reader_states(1);
	reader_states[0].Reader = fReaders[0];	// Select the reader (could be all readers)
	reader_states[0].CurrentState = SCARD_STATE_UNAWARE; // We don't know the state of the card yet

	do
	{
		result = fScardTrack->LocateCards(cards, reader_states); // Locate card
		D(bug("SCARDTRACK::LocateCards()", result));
		
		if ((result == SCARD_E_READER_UNAVAILLABLE) ||
			(result == SCARD_E_UNKNOWN_READER) ||
			(result == SCARD_E_CANCELLED) ||
			(result == SCARD_E_SYSTEM_CANCELLED))
		{ // Can't deal with thoses errors
			fQuitIndicator = true; // We have no choice - ciao.
			return result;
		}

		if (!IsMachineSleeping())
		{
			// Send a message to the BApplication
			notify_application(reader_states[0], true);
			if ((reader_states[0].EventState & SCARD_STATE_ATRMATCH) == SCARD_STATE_ATRMATCH)
			{ // We found a card that matched a known ATR
				D(printf("ATR matched\n"));
				return SCARD_S_SUCCESS;
			}
		} else
			snooze(100000);

		// It seems that there's no card in the reader, that the ATR didn't matched,
		// or the card is already in use.
		// We will wait for a state change, and try again - easy!

		do
		{ // Wait for a card in the reader
			reader_states[0].CurrentState = reader_states[0].EventState; // We know the reader state now.
			result = fScardTrack->GetStatusChange(reader_states, INFINITE); // wait forever for a state change
			D(bug("SCARDTRACK::GetStatusChange()", result));
	
			if ((result == SCARD_E_CANCELLED) ||
				(result == SCARD_E_SYSTEM_CANCELLED))
			{ // System asked us to quit
				fQuitIndicator = true; // We have no choice - ciao.
				return result;
			}

			// Send a message to the BApplication
			if (!IsMachineSleeping())
				notify_application(reader_states[0]);

			// We know the state of the reader now.
			reader_states[0].CurrentState = reader_states[0].EventState;

		} while ((reader_states[0].EventState & SCARD_STATE_PRESENT) != SCARD_STATE_PRESENT);
	
	} while (fQuitIndicator == false);

	return result;
}


RESPONSECODE SCDApplication::connect_card()
{
	// Connect to the reader
	fActiveProtocol = (DWORD)-1;
	RESPONSECODE result = fScardComm->Connect(	fReaders[0],
												SCARD_SHARE_SHARED,	// We want to share the reader
												SCARD_PROTOCOL_T0,	// We only know T=0 protocol
												&fActiveProtocol);	// Will return the active protocol
	D(bug("SCARDCOMM::Connect()", result));
	return result;
}

RESPONSECODE SCDApplication::disconnect_card()
{
	// Connect to the reader
	RESPONSECODE result = fScardComm->Disconnect(SCARD_POWER_DOWN_CARD);
	D(bug("SCARDCOMM::Disconnect()", result));
	return result;
}


RESPONSECODE SCDApplication::wait_remove_card()
{
	RESPONSECODE result = SCARD_E_ERROR;
	STR_LIST cards;
	SCARD_READERSTATE_LIST reader_states(1);
	reader_states[0].Reader = fReaders[0];	// Select the reader (could be all readers)
	reader_states[0].CurrentState = SCARD_STATE_UNAWARE; // We don't know the state of the reader yet

	do
	{
		result = fScardTrack->GetStatusChange(reader_states, INFINITE); // wait forever for a state change
		D(bug("SCARDTRACK::GetStatusChange()", result));
		if (reader_states[0].EventState & SCARD_STATE_EMPTY)
		{ // There's no card in the reader
			D(printf("Card removed\n"));
			result = SCARD_S_SUCCESS;
			break;
		}

		if ((result == SCARD_E_CANCELLED) ||
			(result == SCARD_E_SYSTEM_CANCELLED))
		{ // System asked us to quit
			fQuitIndicator = true; // We have no choice - ciao.
			return result;
		}

		// Send a message to the BApplication
		notify_application(reader_states[0]);

		// We know the reader state now.
		reader_states[0].CurrentState = reader_states[0].EventState;
	} while (fQuitIndicator == false);

	// Clear the current log/pass
	memset(fDefaultLogPass.login, 0, sizeof(fDefaultLogPass.login));
	memset(fDefaultLogPass.password, 0, sizeof(fDefaultLogPass.password));
	return result;
}

status_t SCDApplication::ConnectToServer(const char *login, const char *password)
{
	D(printf("SCDApplication::ConnectToServer(%s, %s)\n", login, password));
#if (_PRODUCT_IAD == 1)
	BMessenger ns(NET_SERVER);	
	if (ns.IsValid())
	{
		BMessage msg(PPP_SMARTCARD_INFO);
		msg.AddString("username", login);
		msg.AddString("password", password);
		return ns.SendMessage(&msg);
	}
	return B_ERROR;	
#else
	return B_OK;	
#endif
}

// -----------------------------------------------------------------------
// #pragma mark -

RESPONSECODE SCDApplication::do_card()
{
	// APDU commands...

#if DEBUG > 0
	// Some data can be freely read
	uint32 w;
	if ((read(0, &w) != SCARD_S_SUCCESS) || (fSw1Sw2 != 0x9000))
		goto error;
	printf("Manufacturer area : %08lX\n", w);
	if ((read(1, &w) != SCARD_S_SUCCESS) || (fSw1Sw2 != 0x9000))
		goto error;
	printf("Issuer area : %08lX\n", w);
	if ((read(2, &w) != SCARD_S_SUCCESS) || (fSw1Sw2 != 0x9000))
		goto error;
	printf("Issuer area : %08lX\n", w);
	if ((read(3, &w) != SCARD_S_SUCCESS) || (fSw1Sw2 != 0x9000))
		goto error;
	printf("Issuer area : %08lX\n", w);
	if ((read(4, &w) != SCARD_S_SUCCESS) || (fSw1Sw2 != 0x9000))
		goto error;
	printf("Issuer area : %08lX\n", w);
	if ((read(5, &w) != SCARD_S_SUCCESS) || (fSw1Sw2 != 0x9000))
		goto error;
	printf("Access conditions : %08lX\n", w);
	if ((read(7, &w) != SCARD_S_SUCCESS) || (fSw1Sw2 != 0x9000))
		goto error;
	printf("CSC 0 Ratification counter 0 : %08lX\n", w);
	if ((read(0x39, &w) != SCARD_S_SUCCESS) || (fSw1Sw2 != 0x9000))
		goto error;
	printf("CSC 1 Ratification counter 1 : %08lX\n", w);
	if ((read(0x3B, &w) != SCARD_S_SUCCESS) || (fSw1Sw2 != 0x9000))
		goto error;
	printf("CSC 2 Ratification counter 2 : %08lX\n", w);
#endif	

	// First, we must present the Card Secret Codes to have
	// read access to the user areas
	
	// Present Card Secret Code 0 (CSC0)
	// to have access to the card
	if ((verify(CSC0, CSC0_VALUE) != SCARD_S_SUCCESS) || (fSw1Sw2 != 0x9000))
		goto error;

	// Present Card Secret Code 1 (CSC1)
	// to have access to the card
	if ((verify(CSC1, CSC1_VALUE) != SCARD_S_SUCCESS) || (fSw1Sw2 != 0x9000))
		goto error;

	// Present Card Secret Code 2 (CSC2)
	// to have access to the card
	if ((verify(CSC2, CSC2_VALUE) != SCARD_S_SUCCESS) || (fSw1Sw2 != 0x9000))
		goto error;

	{ // Here, we can read the user area
		st_card card;
		uint8 *p = (uint8 *)&card;
		for (int i=0 ; i<0xF ; i++)
		{
			uint32 w;
			if ((read(USER_AREA_1 + i, &w) == SCARD_S_SUCCESS) && (fSw1Sw2 == 0x9000))
			{ // Read successfull
				*p++ = (w >> 24) & 0xFF;
				*p++ = (w >> 16) & 0xFF;
				*p++ = (w >>  8) & 0xFF;
				*p++ = w  & 0xFF;
			}
			else
			{ // Read failed
				goto error;
			}
		}
	
		// We got the login and password
		D(printf("Login    : %s\n", card.login));
		D(printf("Password : %s\n", card.password));
		if (ConnectToServer(card.login, card.password) == B_OK)
			fDefaultLogPass = card;
		return SCARD_S_SUCCESS;
	}

error:
	D(printf("Can't read this card\n"));
	BMessage m(MSG_SCARD_USER_UNKNOWN);
	return PostMessage(&m);
}


// -----------------------------------------------------------------------
// #pragma mark -

RESPONSECODE SCDApplication::verify(csc_t csc, uint32 value)
{
	DWORD ResponseSize;
	SCARD_IO_HEADER SendPci;
	BYTE pbReceiveBuffer[255];

	SendPci.Protocol = fActiveProtocol;
	SendPci.Length = sizeof(SendPci);	

	BYTE pbSendBuffer[9];
	pbSendBuffer[0] = 0x00;	// CLA - Not tested by the card/any value is possible
	pbSendBuffer[1] = 0x20;	// INS
	pbSendBuffer[2] = 0x00;	// P1 - Not tested by the card/any value is possible
	pbSendBuffer[3] = csc;	// P2
	pbSendBuffer[4] = 4 ;	// Lc
	pbSendBuffer[5] = value & 0xFF;			// D0
	pbSendBuffer[6] = (value >> 8) & 0xFF;	// D1
	pbSendBuffer[7] = (value >> 16) & 0xFF;	// D2
	pbSendBuffer[8] = (value >> 24) & 0xFF;	// D3

	RESPONSECODE result = fScardComm->Transmit(	SendPci,
												pbSendBuffer,
												sizeof(pbSendBuffer),
												NULL,
												pbReceiveBuffer,
												&ResponseSize);
	D(bug("SCARDCOMM::Transmit()", result));
	D(dump_buffer(pbReceiveBuffer, ResponseSize));

	if ((result == SCARD_S_SUCCESS) && (ResponseSize == 2))
	{
		fSw1 = pbReceiveBuffer[0];
		fSw2 = pbReceiveBuffer[1];
		fSw1Sw2 = (fSw1<<8) | fSw2;
	}

	return result;
}

RESPONSECODE SCDApplication::update(uint8 wordAddr, uint32 word)
{
	DWORD ResponseSize;
	SCARD_IO_HEADER SendPci;
	BYTE pbReceiveBuffer[255];

	SendPci.Protocol = fActiveProtocol;
	SendPci.Length = sizeof(SendPci);	

	BYTE pbSendBuffer[9];
	pbSendBuffer[0] = 0x80;		// CLA - Not tested by the card/any value is possible
	pbSendBuffer[1] = 0xDE;		// INS
	pbSendBuffer[2] = 0x00;		// P1 - Not tested by the card/any value is possible
	pbSendBuffer[3] = wordAddr;	// P2
	pbSendBuffer[4] = 4;		// Lc
	pbSendBuffer[5] = word & 0xFF;			// D0
	pbSendBuffer[6] = (word >> 8) & 0xFF;	// D1
	pbSendBuffer[7] = (word >> 16) & 0xFF;	// D2
	pbSendBuffer[8] = (word >> 24) & 0xFF;	// D3

	RESPONSECODE result = fScardComm->Transmit(	SendPci,
												pbSendBuffer,
												sizeof(pbSendBuffer),
												NULL,
												pbReceiveBuffer,
												&ResponseSize);
	D(bug("SCARDCOMM::Transmit()", result));
	D(dump_buffer(pbReceiveBuffer, ResponseSize));

	if ((result == SCARD_S_SUCCESS) && (ResponseSize == 2))
	{
		fSw1 = pbReceiveBuffer[0];
		fSw2 = pbReceiveBuffer[1];
		fSw1Sw2 = (fSw1<<8) | fSw2;
	}

	return result;
}


RESPONSECODE SCDApplication::read(uint8 wordAddr, uint32 *word)
{
	DWORD ResponseSize;
	SCARD_IO_HEADER SendPci;
	BYTE pbReceiveBuffer[255];

	SendPci.Protocol = fActiveProtocol;
	SendPci.Length = sizeof(SendPci);	

	BYTE pbSendBuffer[5];
	pbSendBuffer[0] = 0x80;		// CLA - Not tested by the card/any value is possible
	pbSendBuffer[1] = 0xBE;		// INS
	pbSendBuffer[2] = 0x00;		// P1 - Not tested by the card/any value is possible
	pbSendBuffer[3] = wordAddr;	// P2
	pbSendBuffer[4] = 4;		// Le

	RESPONSECODE result = fScardComm->Transmit(	SendPci,
												pbSendBuffer,
												sizeof(pbSendBuffer),
												NULL,
												pbReceiveBuffer,
												&ResponseSize);
	D(bug("SCARDCOMM::Transmit()", result));
	D(dump_buffer(pbReceiveBuffer, ResponseSize));

	if ((result == SCARD_S_SUCCESS) && (ResponseSize == 6))
	{
		*word = (pbReceiveBuffer[3] << 24) |
				(pbReceiveBuffer[2] << 16) |
				(pbReceiveBuffer[1] <<  8) |
				(pbReceiveBuffer[0]);
		fSw1 = pbReceiveBuffer[4];
		fSw2 = pbReceiveBuffer[5];
		fSw1Sw2 = (fSw1<<8) | fSw2;
	}

	return result;
}

static bool IsMachineSleeping()
{
	struct stat st;
	return stat("/tmp/sleeping", &st) >= 0;
}


// -----------------------------------------------------------------------
// #pragma mark -

status_t SCDApplication::alert(alert_type alert, int *result)
{
#if (_PRODUCT_IAD == 1)
	BMessage msg_alert(TB_OPEN_ALERT);

	if (fCurrentAlert != ALERT_NO_ALERT)
	{ // There is already an alert on screen!
		if (fCurrentAlert == alert)
		{ // And it's the same that we want to display - no thing to do
			return B_OK;
		}
		// Here, there is already an alert, but, we want to display another one - dissmiss the 1st alert
		// TODO: dissmiss the current alert
		fCurrentAlert = alert;
		return B_OK;
	}

	status_t error;
	fCurrentAlert = alert;
	switch (alert)
	{
		case ALERT_CARD_UNKNOWN:
			msg_alert.AddString("itemplate","Alerts/indirect.txt");
			msg_alert.AddString("template","sc_card_unknown");
			msg_alert.AddString("content","Alerts/smartcard.txt");
			msg_alert.AddString("data","cardunknown");
			msg_alert.AddString("data","OK");
			error=TellTellBrowser(&msg_alert);
			break;

		case ALERT_CARD_WRONG_WAY:
			msg_alert.AddString("itemplate","Alerts/indirect.txt");
			msg_alert.AddString("template","sc_card_wrong_way");
			msg_alert.AddString("content","Alerts/smartcard.txt");
			msg_alert.AddString("data","wrongway");
			msg_alert.AddString("data","OK");
			error=TellTellBrowser(&msg_alert);
			break;
	}
	return error;

#endif
	(void)alert;
	(void)result;
	return B_OK;
}
