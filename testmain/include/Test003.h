#ifndef __TESTMAIN__TEST003_H__
#define __TESTMAIN__TEST003_H__

#include "TestUtility.h"

extern "C" {
	extern TestMainStatus runTest003_01(TestUtility *testUtil, int testCount, pthread_mutex_t *mt, pthread_cond_t *cv, pthread_cond_t *ctDest);
	extern TestMainStatus runTest003_02(TestUtility *testUtil, int testCount, pthread_mutex_t *mt, pthread_cond_t *cv, pthread_cond_t *ctDest);
}

#endif
