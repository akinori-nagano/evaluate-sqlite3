#include <iostream>
#include <stdlib.h>

#include "constants.h"
#include "Test001.h"
#include "Test002.h"
#include "Test.h"

static TestUtility * __unit_init(long tid, const char *logDirPath, const char *logTag);
static void __unit_term(TestUtility *testUtil);

int Sqlite3ThreadSafe(void)
{
	return TestUtility::IsThreadsafe();
}

void TestInit(const char *dbPath, const char *logDirPath)
{
	TestUtility *testUtil = __unit_init(0, logDirPath, "init");

	TestInitialize();

	DBOpen();
	testUtil->deleteAll();
	DBClose();

	__unit_term(testUtil);
}

void Test001_01(const char *dbPath, const char *logDirPath, const char *logTag, int testCount, long tid)
{
	TestUtility *testUtil = __unit_init(tid, logDirPath, logTag);
	if (!testUtil) {
		return;
	}
	__INFO("########## TEST START %ld ##########", tid);

	DBOpen();

	bool isValidTest = true;
	if (runTest001_01(testUtil, testCount)) {
		isValidTest = false;
		goto END;
	}

END:
	DBClose();
	if (isValidTest) {
		__INFO("########## TEST END ##########");
	} else {
		__FATAL("########## TEST FAILED ##########");
	}
	__unit_term(testUtil);
}

void Test002_01(const char *dbPath, const char *logDirPath, const char *logTag, int testCount, long tid)
{
	TestUtility *testUtil = __unit_init(tid, logDirPath, logTag);
	if (!testUtil) {
		return;
	}
	__INFO("########## TEST START %ld ##########", tid);

	DBOpen();

	bool isValidTest = true;
	if (runTest002_01(testUtil, testCount)) {
		isValidTest = false;
		goto END;
	}

END:
	DBClose();
	if (isValidTest) {
		__INFO("########## TEST END ##########");
	} else {
		__FATAL("########## TEST FAILED ##########");
	}
	__unit_term(testUtil);
}

void Test002_02(const char *dbPath, const char *logDirPath, const char *logTag, int testCount, long tid)
{
	TestUtility *testUtil = __unit_init(tid, logDirPath, logTag);
	if (!testUtil) {
		return;
	}
	__INFO("########## TEST START %ld ##########", tid);

	DBOpen();

	bool isValidTest = true;
	if (runTest002_02(testUtil, testCount)) {
		isValidTest = false;
		goto END;
	}

END:
	DBClose();
	if (isValidTest) {
		__INFO("########## TEST END ##########");
	} else {
		__FATAL("########## TEST FAILED ##########");
	}
	__unit_term(testUtil);
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
