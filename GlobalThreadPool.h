#pragma once

#include <ThreadPool/ThreadPool.h>
#include <thread>

static ThreadPool globalThreadPool(std::thread::hardware_concurrency() - 4);