/*
 *	Filename: event.h
 * Description: ÀÌº¥Æ® °ü·Ã (timed event)
 *
 *	  Author: ±èÇÑÁÖ (aka. ºñ¿±, Cronan), ¼Û¿µÁø (aka. myevan, ºøÀÚ·ç)
 */
#ifndef __INC_LIBTHECORE_EVENT_H__
#define __INC_LIBTHECORE_EVENT_H__

#include <boost/intrusive_ptr.hpp>

/**
 * Base class for all event info data
 */
struct event_info_data 
{
	event_info_data() {}
	virtual ~event_info_data() {}
};
	
typedef struct event EVENT;
typedef boost::intrusive_ptr<EVENT> LPEVENT;
typedef long (*TEVENTFUNC) (LPEVENT event, long processing_time);

#define EVENTFUNC(name)	long (name) (LPEVENT event, long processing_time)
#define EVENTINFO(name) struct name : public event_info_data

struct TQueueElement;

struct event
{
	event() : func(NULL), info(NULL), q_el(NULL), ref_count(0) {}
	~event() {
		if (info != NULL) {
			M2_DELETE(info);
		}
	}
	TEVENTFUNC			func;
	event_info_data* 	info;
	TQueueElement *		q_el;
	char				is_force_to_end;
	char				is_processing;

	size_t ref_count;
};

extern void intrusive_ptr_add_ref(EVENT* p);
extern void intrusive_ptr_release(EVENT* p);

template<class T> // T should be a subclass of event_info_data
T* AllocEventInfo() {
	return M2_NEW T;
}

extern void		event_destroy();
extern int		event_process(int pulse);
extern int		event_count();

#define event_create(func, info, when) event_create_ex(func, info, when)
extern LPEVENT	event_create_ex(TEVENTFUNC func, event_info_data* info, long when);
extern void		event_cancel(LPEVENT * event);			// ÀÌº¥Æ® Ãë¼Ò
extern long		event_processing_time(LPEVENT event);	// ¼öÇà ½Ã°£ ¸®ÅÏ
extern long		event_time(LPEVENT event);			// ³²Àº ½Ã°£ ¸®ÅÏ
extern void		event_reset_time(LPEVENT event, long when);	// ½ÇÇà ½Ã°£ Àç ¼³Á¤
extern void		event_set_verbose(int level);

extern event_info_data* FindEventInfo(DWORD dwID);
extern event_info_data*	event_info(LPEVENT event);

#endif
