#pragma once 

#include "Socket.h"
#include "Endpoint.h"
#include "Exception.h"
#include "UtilityMacros.h"
#include "Iocp.h"
#include "ServerFlag.h"
#include "ServerStruct.h"

#include <mutex>
#include <queue>
#include <chrono>
#include <wincrypt.h>
