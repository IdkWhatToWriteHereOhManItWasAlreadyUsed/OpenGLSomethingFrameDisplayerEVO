#pragma once

#include <ThreadPool/ThreadPool.h>
#include <thread>

namespace OpenGLSomethingFrameDisplayerEVO {

static ThreadPool globalThreadPool(std::thread::hardware_concurrency() - 4);

}