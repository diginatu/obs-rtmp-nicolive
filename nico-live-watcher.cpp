#include <QtCore>
#include "nicolive.h"
#include "nico-live.hpp"
#include "nico-live-watcher.hpp"
#include "nicolive-ui.h"

NicoLiveWatcher::NicoLiveWatcher(NicoLive *nicolive, int margin_sec) :
	QObject(nicolive),
	nicolive(nicolive),
	marginTime(margin_sec * 1000)
{
	this->timer = new QTimer(this);
	this->timer->setSingleShot(true);
	connect(timer, SIGNAL(timeout()), this, SLOT(watch()));
}

NicoLiveWatcher::~NicoLiveWatcher()
{
	if (isActive())
		stop();
}

void NicoLiveWatcher::start(int sec)
{
	this->interval = sec * 1000;
	if (this->interval < NicoLiveWatcher::MIN_INTERVAL)
		this->interval = NicoLiveWatcher::MIN_INTERVAL;
	if (!this->timer->isActive()) {
		nicolive_log_debug("check session before timer start");
		nicolive->checkSession();
		nicolive_log_debug("start watch, interval: %d", this->interval);
		// this->timer->start(this->interval);
		this->timer->start(this->marginTime);
	}
	this->active = true;
}

void NicoLiveWatcher::stop()
{
	if (this->timer->isActive()) {
		nicolive_log_debug("stop watch ");
		this->timer->stop();
	}
	this->active = false;
}

bool NicoLiveWatcher::isActive()
{
	return this->active;
}

int NicoLiveWatcher::remainingTime()
{
	return this->timer->remainingTime() / 1000;
}

void NicoLiveWatcher::watch()
{
	nicolive_log_debug("watching!");

	int next_interval = this->interval;
	int remaining_msec;

	nicolive->sitePubStat();
	remaining_msec = nicolive->getRemainingLive() * 1000;
	if (remaining_msec < 0)
		remaining_msec = 0;

	if (nicolive->getLiveId().isEmpty()) {
		if (nicolive->isOnair()) {
			nicolive_streaming_click();
			next_interval = this->marginTime;
		}
	} else {
		if (nicolive->getLiveId() != nicolive->getOnairLiveId()) {
			if (nicolive->isOnair()) {
				nicolive_streaming_click();
				QThread::sleep(1); // sleep 1 sec
			}
			nicolive_streaming_click();
		} else if (remaining_msec + this->marginTime < next_interval) {
			next_interval = remaining_msec + this->marginTime;
		}
	}
	this->timer->start(next_interval);
}