#include "DD_EventQueue.h"
#include "DD_Terminal.h"

bool DD_Queue::push(const DD_Event & _event)
{
	size_t qsize = m_data.size();
	if( m_numEvents == qsize ) {
		return false;
	}
	m_data[m_tail] = std::move(_event);
	m_tail = (m_tail + 1) % qsize;
	m_numEvents++;
	return true;
}

bool DD_Queue::pop(DD_Event & _event)
{
	size_t qsize = m_data.size();
	if( m_numEvents == 0 ) {
		return false;
	}
	_event = std::move(m_data[m_head]);
	m_head = (m_head + 1) % qsize;
	m_numEvents--;
	return true;
}

bool DD_Queue::RegisterHandler(EventHandler& handler, 
							   const char * ticket,
							   const char* sig)
{
	if( handlers.find(ticket) == handlers.end() ) {
		// if it doesn't exist, create new container
		handlers[ticket] = callbackContainer(50);
		m_counter[ticket] = 1;
		handlers[ticket][0].func = handler;
		handlers[ticket][0].sig.set(sig);
		return true;
	}
	else {
		if( m_counter[ticket] >= handlers[ticket].size() ) {
			// no more space, create more
			callbackContainer temp(m_counter[ticket] + 25);
			temp = handlers[ticket];
			handlers[ticket] = std::move(temp);
		}
		// add new handler
		handlers[ticket][m_counter[ticket]].func = handler;
		handlers[ticket][m_counter[ticket]].sig.set(sig);
		m_counter[ticket] += 1;
		return true;
	}
}

// Registers function that post events into the queue
bool DD_Queue::RegisterPoster(EventHandler & handler, const char* sig)
{
	if( m_postLimit >= m_postHandlers.size() ) {
		return false;
	}
	m_postHandlers[m_postLimit].func = handler;
	m_postHandlers[m_postLimit].sig.set(sig);
	m_postLimit += 1;
	return true;
}

// Queries for post events and loads the queue before render job is sent
void DD_Queue::GetPosts(const char* postID, const float dt, const float totalTime)
{
	for( size_t i = 0; i < m_postLimit; i++ ) {
		DD_Event event = DD_Event();
		const float frame_t = main_clock->getFrameTime();
		const float run_t = main_clock->getTimeFloat();
		event.m_type = postID;
		event.m_time = frame_t;
		event.m_total_runtime = run_t;

		DD_Event newEvent = std::move(m_postHandlers[i].func(event));

		if (newEvent.m_type.compare("") != 0) { push(newEvent); }
	}
}

void DD_Queue::ProcessQueue()
{
	const float frame_t = main_clock->getFrameTime();
	const float run_t = main_clock->getTimeFloat();

	const size_t events_this_frame = m_numEvents;
	for( size_t i = 0; i < events_this_frame; i++ ) {
		DD_Event current = DD_Event();
		pop(current);
		current.m_time = frame_t;
		current.m_total_runtime = run_t;

		// find and loop through array that matches
		if( handlers.find(current.m_type) != handlers.end() ) {
			callbackContainer& pool = handlers[current.m_type];
			size_t count = m_counter[current.m_type];

			for( size_t j = 0; j < count; j++ ) {
				DD_Event new_event = pool[j].func(current);	// process event
				if (new_event.m_type.compare("") != 0) {	// push new event
					push(new_event);
				}
			}
		}

		if( current.m_message != nullptr ) {
			delete current.m_message;
		}
	}
}
