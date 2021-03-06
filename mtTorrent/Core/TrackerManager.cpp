#include "TrackerManager.h"
#include "HttpTrackerComm.h"
#include "UdpTrackerComm.h"
#include "Configuration.h"
#include "Torrent.h"

mtt::TrackerManager::TrackerManager(TorrentPtr t) : torrent(t)
{
}

std::string cutStringPart(std::string& source, DataBuffer endChars, int cutAdd)
{
	size_t id = std::string::npos;

	for (auto c : endChars)
	{
		auto nid = source.find(c);

		if (nid < id)
			id = nid;
	}

	std::string ret = source.substr(0, id);

	if (id == std::string::npos)
		source = "";
	else
		source = source.substr(id + 1 + cutAdd, std::string::npos);

	return ret;
}

void mtt::TrackerManager::start(AnnounceCallback callbk)
{
	std::lock_guard<std::mutex> guard(trackersMutex);

	announceCallback = callbk;

	for (size_t i = 0; i < 3 && i < trackers.size(); i++)
	{
		auto& t = trackers[i];

		if (!t.comm)
		{
			start(&t);
		}
	}
}

void mtt::TrackerManager::stop()
{
	std::lock_guard<std::mutex> guard(trackersMutex);

	stopAll();

	announceCallback = nullptr;
}

void mtt::TrackerManager::addTracker(std::string addr)
{
	std::lock_guard<std::mutex> guard(trackersMutex);

	TrackerInfo info;

	info.protocol = cutStringPart(addr, { ':' }, 2);
	info.host = cutStringPart(addr, { ':', '/' }, 0);
	info.port = cutStringPart(addr, { '/' }, 0);

	if (!info.port.empty())
	{
		auto i = findTrackerInfo(info.host);

		if (i && i->port == info.port)
		{
			if ((i->protocol == "http" || info.protocol == "http") && (i->protocol == "udp" || info.protocol == "udp") && !i->comm)
			{
				i->protocol = "udp";
				i->httpFallback = true;
			}
		}
		else
			trackers.push_back(info);
	}
}

void mtt::TrackerManager::addTrackers(const std::vector<std::string>& trackers)
{
	for (auto& t : trackers)
	{
		addTracker(t);
	}
}

void mtt::TrackerManager::removeTracker(const std::string& addr)
{
	std::lock_guard<std::mutex> guard(trackersMutex);

	for (auto it = trackers.begin(); it != trackers.end(); it++)
	{
		if (it->host == addr)
		{
			trackers.erase(it);
			break;
		}
	}
}

void mtt::TrackerManager::removeTrackers()
{
	std::lock_guard<std::mutex> guard(trackersMutex);
	trackers.clear();
}

std::shared_ptr<mtt::Tracker> mtt::TrackerManager::getTracker(const std::string& addr)
{
	if (auto info = findTrackerInfo(addr))
	{
		return info->comm;
	}

	return nullptr;
}

std::vector<std::shared_ptr<mtt::Tracker>> mtt::TrackerManager::getTrackers()
{
	std::vector<std::shared_ptr<mtt::Tracker>> out;

	std::lock_guard<std::mutex> guard(trackersMutex);

	for (auto& t : trackers)
	{
		if(t.comm)
			out.push_back(t.comm);
	}

	return out;
}

std::vector<std::string> mtt::TrackerManager::getTrackersList()
{
	std::vector<std::string> out;

	std::lock_guard<std::mutex> guard(trackersMutex);

	for (auto& t : trackers)
	{
		out.push_back(t.host);
	}

	return out;
}

uint32_t mtt::TrackerManager::getTrackersCount()
{
	return (uint32_t)trackers.size();
}

void mtt::TrackerManager::onAnnounce(AnnounceResponse& resp, Tracker* t)
{
	torrent->service.io.post([this, resp, t]()
		{
			std::lock_guard<std::mutex> guard(trackersMutex);

			if (auto trackerInfo = findTrackerInfo(t))
			{
				trackerInfo->retryCount = 0;
				trackerInfo->timer->schedule(resp.interval);
				trackerInfo->comm->info.nextAnnounce = (uint32_t)time(0) + resp.interval;
			}

			if (announceCallback)
				announceCallback(Status::Success, &resp, t);

			startNext();
		});
}

void mtt::TrackerManager::onTrackerFail(Tracker* t)
{
	std::lock_guard<std::mutex> guard(trackersMutex);

	if (auto trackerInfo = findTrackerInfo(t))
	{
		if (trackerInfo->httpFallback && !trackerInfo->httpFallbackUsed && trackerInfo->protocol == "udp")
		{
			trackerInfo->httpFallbackUsed = true;
			trackerInfo->protocol = "http";
			start(trackerInfo);
		}
		else
		{
			if (trackerInfo->httpFallback && trackerInfo->httpFallbackUsed && trackerInfo->protocol == "http")
			{
				trackerInfo->protocol = "udp";
				start(trackerInfo);
			}
			else
			{
				trackerInfo->retryCount++;
				uint32_t nextRetry = 30 * trackerInfo->retryCount;
				trackerInfo->timer->schedule(nextRetry);
				trackerInfo->comm->info.nextAnnounce = (uint32_t)time(0) + nextRetry;
			}

			startNext();
		}

		if(announceCallback)
			announceCallback(Status::E_Unknown, nullptr, t);
	}
}

void mtt::TrackerManager::start(TrackerInfo* tracker)
{
	if (tracker->protocol == "udp")
		tracker->comm = std::make_shared<UdpTrackerComm>();
	else if (tracker->protocol == "http")
		tracker->comm = std::make_shared<HttpTrackerComm>();
	else
		return;

	tracker->comm->onFail = std::bind(&TrackerManager::onTrackerFail, this, tracker->comm.get());
	tracker->comm->onAnnounceResult = std::bind(&TrackerManager::onAnnounce, this, std::placeholders::_1, tracker->comm.get());

	tracker->comm->init(tracker->host, tracker->port, torrent);
	tracker->timer = ScheduledTimer::create(torrent->service.io, std::bind(&Tracker::announce, tracker->comm.get()));
	tracker->retryCount = 0;

	tracker->comm->announce();
}

void mtt::TrackerManager::startNext()
{
	for (auto& tracker : trackers)
	{
		if (!tracker.comm)
			start(&tracker);
	}
}

void mtt::TrackerManager::stopAll()
{
	for (auto& tracker : trackers)
	{
		if(tracker.comm)
			tracker.comm->deinit();
		tracker.comm = nullptr;
		if(tracker.timer)
			tracker.timer->disable();
		tracker.timer = nullptr;
		tracker.retryCount = 0;
	}
}

mtt::TrackerManager::TrackerInfo* mtt::TrackerManager::findTrackerInfo(Tracker* t)
{
	for (auto& tracker : trackers)
	{
		if (tracker.comm.get() == t)
			return &tracker;
	}

	return nullptr;
}

mtt::TrackerManager::TrackerInfo* mtt::TrackerManager::findTrackerInfo(std::string host)
{
	for (auto& tracker : trackers)
	{
		if (tracker.host == host)
			return &tracker;
	}

	return nullptr;
}
