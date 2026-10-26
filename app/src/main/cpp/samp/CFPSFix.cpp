#include "CFPSFix.h"
#include "main.h"
#include <sys/syscall.h>
#include "game/game.h"

static void setThreadAffinityMask(pid_t tid, uint32_t mask)
{
	syscall(__NR_sched_setaffinity, tid, sizeof(mask), &mask);
}

void CFPSFix::Routine()
{
	while (true)
	{
		m_Mutex.lock();
		for (auto& i : m_Threads)
		{
			uint32_t mask = 0xff;

			setThreadAffinityMask(i, mask);
		}
		m_Mutex.unlock();
	}
}

CFPSFix::CFPSFix()
{
	std::thread(&CFPSFix::Routine, this).detach();
}

CFPSFix::~CFPSFix()
{
}

void CFPSFix::PushThread(pid_t tid)
{
	std::lock_guard<std::mutex> lock(m_Mutex);

	m_Threads.push_back(tid);
}