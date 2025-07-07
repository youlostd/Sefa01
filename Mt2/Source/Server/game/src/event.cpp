/*
 *	Filename: event.c
 * Description: ÀÌº¥Æ® °ü·Ã (timed event)
 *
 *	  Author: ±èÇÑÁÖ (aka. ºñ¿±, Cronan), ¼Û¿µÁø (aka. myevan, ºøÀÚ·ç)
 */
#include "stdafx.h"

#include "event_queue.h"

extern void ShutdownOnFatalError();

static CEventQueue cxx_q;

/* ÀÌº¥Æ®¸¦ »ý¼ºÇÏ°í ¸®ÅÏÇÑ´Ù */
LPEVENT event_create_ex(TEVENTFUNC func, event_info_data* info, long when)
{
	LPEVENT new_event = NULL;

	/* ¹Ýµå½Ã ´ÙÀ½ pulse ÀÌ»óÀÇ ½Ã°£ÀÌ Áö³­ ÈÄ¿¡ ºÎ¸£µµ·Ï ÇÑ´Ù. */
	if (when < 1)
		when = 1;

	new_event = M2_NEW event;
	assert(NULL != new_event);

	new_event->func = func;
	new_event->info	= info;
	new_event->q_el	= cxx_q.Enqueue(new_event, when, thecore_heart->pulse);
	new_event->is_processing = FALSE;
	new_event->is_force_to_end = FALSE;

	return (new_event);
}

/* ½Ã½ºÅÛÀ¸·Î ºÎÅÍ ÀÌº¥Æ®¸¦ Á¦°ÅÇÑ´Ù */
void event_cancel(LPEVENT * ppevent)
{
	LPEVENT event;

	if (!ppevent)
	{
		sys_err("null pointer");
		return;
	}

	if (!(event = *ppevent))
		return;

	if (event->is_processing)
	{
		event->is_force_to_end = TRUE;

		if (event->q_el)
			event->q_el->bCancel = TRUE;

		*ppevent = NULL;
		return;
	}

	// ÀÌ¹Ì Ãë¼Ò µÇ¾ú´Â°¡?
	if (!event->q_el)
	{
		*ppevent = NULL;
		return;
	}

	if (event->q_el->bCancel)
	{
		*ppevent = NULL;
		return;
	}

	event->q_el->bCancel = TRUE;

	*ppevent = NULL;
}

void event_reset_time(LPEVENT event, long when)
{
	if (!event->is_processing)
	{
		if (event->q_el)
			event->q_el->bCancel = TRUE;

		event->q_el = cxx_q.Enqueue(event, when, thecore_heart->pulse);
	}
}

/* ÀÌº¥Æ®¸¦ ½ÇÇàÇÒ ½Ã°£¿¡ µµ´ÞÇÑ ÀÌº¥Æ®µéÀ» ½ÇÇàÇÑ´Ù */
int event_process(int pulse)
{
	long	new_time;
	int		num_events = 0;

	// event_q Áï ÀÌº¥Æ® Å¥ÀÇ ÇìµåÀÇ ½Ã°£º¸´Ù ÇöÀçÀÇ pulse °¡ ÀûÀ¸¸é ·çÇÁ¹®ÀÌ 
	// µ¹Áö ¾Ê°Ô µÈ´Ù.
	while (pulse >= cxx_q.GetTopKey())
	{
		TQueueElement * pElem = cxx_q.Dequeue();

		if (pElem->bCancel)
		{
			cxx_q.Delete(pElem);
			continue;
		}

		new_time = pElem->iKey;

		LPEVENT the_event = pElem->pvData;
		long processing_time = event_processing_time(the_event);
		cxx_q.Delete(pElem);

		/*
		 * ¸®ÅÏ °ªÀº »õ·Î¿î ½Ã°£ÀÌ¸ç ¸®ÅÏ °ªÀÌ 0 º¸´Ù Å¬ °æ¿ì ÀÌº¥Æ®¸¦ ´Ù½Ã Ãß°¡ÇÑ´Ù. 
		 * ¸®ÅÏ °ªÀ» 0 ÀÌ»óÀ¸·Î ÇÒ °æ¿ì event ¿¡ ÇÒ´çµÈ ¸Þ¸ð¸® Á¤º¸¸¦ »èÁ¦ÇÏÁö ¾Êµµ·Ï
		 * ÁÖÀÇÇÑ´Ù.
		 */
		the_event->is_processing = TRUE;

		if (!the_event->info)
		{
			the_event->q_el = NULL;
		}
		else
		{
			the_event->q_el = NULL;

			new_time = (the_event->func) (get_pointer(the_event), processing_time);
			
			if (new_time > 0 && !the_event->is_force_to_end)
			{
				the_event->q_el = cxx_q.Enqueue(the_event, new_time, pulse);
				the_event->is_processing = FALSE;
			}
		}

		++num_events;
	}

	return num_events;
}

/* ÀÌº¥Æ®°¡ ¼öÇà½Ã°£À» pulse ´ÜÀ§·Î ¸®ÅÏÇØ ÁØ´Ù */
long event_processing_time(LPEVENT event)
{
	long start_time;

	if (!event->q_el)
		return 0;

	start_time = event->q_el->iStartTime;
	return (thecore_heart->pulse - start_time);
}

/* ÀÌº¥Æ®°¡ ³²Àº ½Ã°£À» pulse ´ÜÀ§·Î ¸®ÅÏÇØ ÁØ´Ù */
long event_time(LPEVENT event)
{
	long when;

	if (!event->q_el)
		return 0;

	when = event->q_el->iKey;
	return (when - thecore_heart->pulse);
}

/* ¸ðµç ÀÌº¥Æ®¸¦ Á¦°ÅÇÑ´Ù */
void event_destroy(void)
{
	TQueueElement * pElem;

	while ((pElem = cxx_q.Dequeue()))
	{
		LPEVENT the_event = (LPEVENT) pElem->pvData;

		if (!pElem->bCancel)
		{
			// no op here
		}

		cxx_q.Delete(pElem);
	}
}

int event_count()
{
	return cxx_q.Size();
}

void intrusive_ptr_add_ref(EVENT* p) {
	++(p->ref_count);
}

void intrusive_ptr_release(EVENT* p) {
	if ( --(p->ref_count) == 0 ) {
		M2_DELETE(p);
	}
}
