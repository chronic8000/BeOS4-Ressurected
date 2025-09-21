/************************************************************************/
/* copyright 2000 Be, Incorporated. All rights reserved.                */
/************************************************************************/

#include "ua100.h"

#define ddprintf dprintf

static bool
fill_chunk(parser_state* s, int32 head, int32 one, int32 two, int32 three)
{
  midi_chunk* ch = s->ch;
  s->data = -1;
  s->sx_data = -1;
  s->sx_sent = true;
  ch->head = head;
  ch->data[0] = one;
  ch->data[1] = two;
  ch->data[2] = three;
  return true;
}

static bool
status_byte(parser_state* s, uint8 c)
{
  if (c < 0xf6) {
	// has data
	s->status = c;
	s->data = -1;
	s->sx_sent = false;
	return false;
  }
  if (c == 0xf7 && s->status == 0xf0) {
	// end of sysex
	s->status = 0;
	if (s->sx_sent) {
	  if (s->sx_data >= 0)
		return fill_chunk(s, 7, s->data, s->sx_data, c);
	  if (s->data >= 0)
		return fill_chunk(s, 6, s->data, c, 0);
	  else
		return fill_chunk(s, 5, c, 0, 0);
	}
	if (s->data >= 0)
	  return fill_chunk(s, 7, s->status, s->data, c);
	else
	  return fill_chunk(s, 6, s->status, c, 0);
  }
  if (c < 0xf8) {
	// no data
	s->status = 0;
	return fill_chunk(s, 5, c, 0, 0);
  }

  // real time message
  s->ch->head = 15;
  s->ch->data[0] = c;
  return true;
}

static bool
sx_data_byte(parser_state* s, uint8 c)
{
  if (s->sx_sent) {
	if (s->sx_data >= 0)
	  return fill_chunk(s, 4, s->data, s->sx_data, c);
	if (s->data >= 0)
	  s->sx_data = c;
	else
	  s->data = c;
	return false;
  }
  if (s->data >= 0)
	return fill_chunk(s, 4, s->status, s->data, c);
  s->data = c;
  return false;
}

static bool
data_byte(parser_state* s, uint8 c)
{
  int32 msg = (s->status & 0xf0) >> 4;

  if (msg < 8)
	return false;

  if (msg == 12 || msg == 13)
	// two byte channel message
	return fill_chunk(s, msg, s->status, c, 0);

  if (msg < 15) {
	// three byte channel message
	if (s->data >= 0)
	  return fill_chunk(s, msg, s->status, s->data, c);
	s->data = c;
	return false;
  }

  switch (s->status) {
  case 0xf0:
	// system exclusive
	return sx_data_byte(s, c);
  case 0xf1:
  case 0xf3:
	// two byte system message
	return fill_chunk(s, 2, s->status, c, 0);
  case 0xf5:
	// cable message
	if (s->data != 0x7e)
	  return fill_chunk(s, 2, s->status, c, 0);
	// fall through for third byte
  case 0xf2:
	// three byte system message
	if (s->data >= 0)
	  return fill_chunk(s, 3, s->status, s->data, c);
	s->data = c;
	return false;
  }

  return false;
}

int32
parse_midi(midi_channel* chan, midi_chunk* ch)
{
  parser_state* s = &chan->state;
  uint32 sent_count = 0;

  *(int32*) ch = 0;
  s->ch = ch;

  while (chan->sendq_count > 0) {
	uint8 c = chan->sendq_bufr[chan->sendq_tail];
	if (c & 0x80 && s->status == 0xf0 && c != 0xf7) {
	  // terminated sysex (even real-time events terminate sysex)
	  status_byte(s, 0xf7);
	  break;
	}
	chan->sendq_tail = (chan->sendq_tail + 1) & SEND_MASK;
	--chan->sendq_count;
	++sent_count;
	if (c & 0x80 ? status_byte(s, c) : data_byte(s, c))
	  break;
  }

  return sent_count;
}

