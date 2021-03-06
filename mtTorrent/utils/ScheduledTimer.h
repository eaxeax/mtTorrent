#pragma once
#include <asio.hpp>
#include <functional>

struct ScheduledTimer : public std::enable_shared_from_this<ScheduledTimer>
{
	static std::shared_ptr<ScheduledTimer> create(asio::io_service& io, std::function<void()> callback);

	ScheduledTimer(asio::io_service& io, std::function<void()> callback);
	~ScheduledTimer();

	void schedule(uint32_t secondsOffset);
	void disable();
	uint32_t getSecondsTillNextUpdate();

private:

	void checkTimer(const asio::error_code& error);
	std::function<void()> func;
	std::unique_ptr<asio::steady_timer> timer;
	std::mutex mtx;
};