#pragma once

#include <gnunet/gnunet_core_service.h>
#include <gnunet/gnunet_util_lib.h>

#include <string>
#include <functional>
#include <utility>
#include <stdexcept>
#include <chrono>
#include <mutex>

// Fundamental features of the library
#include "gnunetpp-scheduler.hpp"
#include "inner/coroutine.hpp"