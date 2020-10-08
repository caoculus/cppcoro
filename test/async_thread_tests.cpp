#include "doctest/doctest.h"

#include <cppcoro/async_thread.hpp>
#include <cppcoro/io_service.hpp>
#include <cppcoro/on_scope_exit.hpp>
#include <cppcoro/sync_wait.hpp>
#include <cppcoro/task.hpp>
#include <cppcoro/when_all.hpp>

#include <cppcoro/cancellation_source.hpp>

using namespace cppcoro;

TEST_SUITE_BEGIN("async_thread");

TEST_CASE("join")
{
	io_service service;

	sync_wait(when_all(
		[&]() -> task<> {
			using namespace std::literals;
			auto _ = on_scope_exit([&] { service.stop(); });
			int the_response = -1;
			async_thread async_thread{ service, [&the_response] {
										  //										  std::this_thread::sleep_for(50ms);
										  the_response = 42;
									  } };
			co_await async_thread.join();
			REQUIRE(the_response == 42);
		}(),
		[&]() -> task<> {
			service.process_events();
			co_return;
		}()));
}

 TEST_CASE("cancel")
{
	io_service service;

	sync_wait(when_all(
		[&]() -> task<> {
			auto _ = on_scope_exit([&] { service.stop(); });
			int the_response = -1;
			cancellation_source cancellation_source;
			async_thread async_thread{ service,
									   [&the_response](cancellation_token ct) {
										   using namespace std::literals;
										   std::this_thread::sleep_for(20ms);
										   if (ct.is_cancellation_requested())
											   return;
										   the_response = 42;
									   },
									   cancellation_source.token() };
			cancellation_source.request_cancellation();
			co_await async_thread.join();
			REQUIRE(the_response == -1);
		}(),
		[&]() -> task<> {
			service.process_events();
			co_return;
		}()));
}

TEST_SUITE_END();
