#include <iostream>
#include <stdlib.h>
#include <pthread.h>

#include "constants.h"
#include "Test001.h"
#include "Test002.h"
#include "Test003.h"
#include "Test.h"

static TestMainStatus __pthread_mutex_cond_init(TestUtility *testUtil, pthread_mutex_t *mt, pthread_cond_t *cv);
static TestUtility * __unit_init(long tid, const char *logDirPath, const char *logTag);
static void __unit_term(TestUtility *testUtil);


static int mtxInitialized = 0;
static pthread_mutex_t mtxTest003_01;
static pthread_cond_t condTest003_01;
static pthread_mutex_t mtxTest003_02;
static pthread_cond_t condTest003_02;


int Sqlite3ThreadSafe(void)
{
	return TestUtility::IsThreadsafe();
}

int
TestInit(const char *dbPath, const char *logDirPath)
{
	TestUtility *testUtil = __unit_init(0, logDirPath, "init");
	if (!testUtil) {
		return 1;
	}

	TestInitialize();

	if (!mtxInitialized) {
		if (__pthread_mutex_cond_init(testUtil, &mtxTest003_01, &condTest003_01)) {
			__FATAL("Test003_01 mutex_cond_init failed.");
			return 1;
		}
		if (__pthread_mutex_cond_init(testUtil, &mtxTest003_02, &condTest003_02)) {
			__FATAL("Test003_02 mutex_cond_init failed.");
			return 1;
		}
		mtxInitialized = 1;
	}

	DBOpen();
	testUtil->deleteAll();
	DBClose();

	__unit_term(testUtil);
	return 0;
}

int
TestExec(const char *dbPath, const char *logDirPath, const char *logTag, int kind, int testCount, long tid)
{
	TestUtility *testUtil = __unit_init(tid, logDirPath, logTag);
	if (!testUtil) {
		return 1;
	}
	__INFO("########## TEST START %ld ##########", tid);

	DBOpen();

	bool isValidTest = true;
	TestMainStatus st = TestMainStatus::Ok;
	switch (kind) {
		case ExecKind_Test001_01: st = runTest001_01(testUtil, testCount); break;
		case ExecKind_Test002_01: st = runTest002_01(testUtil, testCount); break;
		case ExecKind_Test002_02: st = runTest002_02(testUtil, testCount); break;
		case ExecKind_Test003_01: st = runTest003_01(testUtil, testCount, &mtxTest003_01, &condTest003_01, &condTest003_02); break;
		case ExecKind_Test003_02: st = runTest003_02(testUtil, testCount, &mtxTest003_02, &condTest003_02, &condTest003_01); break;
		default:
			__FATAL("unknown kind. (kind=%d)", kind);
			st = TestMainStatus::Error;
			break;
	}
	if (st) {
		isValidTest = false;
		goto END;
	}

END:
	DBClose();

	if (isValidTest) {
		__INFO("########## TEST END ##########");
		return 0;
	}
	__FATAL("########## TEST FAILED ##########");

	__unit_term(testUtil);
	return 1;
}

TestMainStatus
__pthread_mutex_cond_init(TestUtility *testUtil, pthread_mutex_t *mt, pthread_cond_t *cv)
{
	int ret = 0;

	ret = pthread_mutex_init(mt, NULL);
	if (ret) {
		__FATAL("pthread_mutex_init failed. (ret=%d)(errno=%d)", ret, errno);
		return TestMainStatus::Error;
	}

	ret = ret = pthread_cond_init(cv, NULL);
	if (ret) {
		__FATAL("pthread_cond_init failed. (ret=%d)(errno=%d)", ret, errno);
		return TestMainStatus::Error;
	}
	return TestMainStatus::Ok;
}

TestUtility *
__unit_init(long tid, const char *logDirPath, const char *logTag)
{
	char buf[1024];
	snprintf(buf, sizeof(buf), "%s%s-%ld.txt", logDirPath, logTag, tid);
	FILE *fp = fopen(buf, "w");
	if (fp == NULL) {
		std::cout << __FUNCTION__ << ":FATAL: cannot open logfile. [" << buf << "]" << std::endl;
		return NULL;
	}

	return new TestUtility(fp, tid);
}

void
__unit_term(TestUtility *testUtil)
{
	FILE *fp = testUtil->fpDebug;
	delete testUtil;
	if (fp) {
		fclose(fp);
	}
}
