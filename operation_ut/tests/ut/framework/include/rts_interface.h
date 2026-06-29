#ifndef _OP_API_UT_COMMON_RTS_INTERFACE_H_
#define _OP_API_UT_COMMON_RTS_INTERFACE_H_

#include <cstdlib>
#include <iostream>
#include <map>
#include "hacl_rt.h"

#define DEVICE_ID 0

using namespace std;

int RtsInit();

void RtsUnInit();

void RtsCreateStream(rtStream_t *stream);

int SynchronizeStream(rtStream_t stream);

#endif
