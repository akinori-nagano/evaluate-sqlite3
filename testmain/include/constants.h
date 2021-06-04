#ifndef __TESTMAIN__CONSTANTS_H__
#define __TESTMAIN__CONSTANTS_H__

#define __INFO(fmt, ...)  testUtil->debugOut("INFO",  __FUNCTION__, fmt, ##__VA_ARGS__)
#define __ERROR(fmt, ...) testUtil->debugOut("ERROR", __FUNCTION__, fmt, ##__VA_ARGS__)
#define __FATAL(fmt, ...) testUtil->debugOut("FATAL", __FUNCTION__, fmt, ##__VA_ARGS__)
#define __ASSERT(fmt, ...) testUtil->debugOut("ASSERT", __FUNCTION__, fmt, ##__VA_ARGS__)

#define DBOpen() { \
	if (testUtil->openDB(__FUNCTION__, dbPath) != TestMainStatus::Ok) { \
		__FATAL("db open failed."); \
		return; \
	} \
}
#define DBClose() { \
	testUtil->closeDB(__FUNCTION__); \
}

#define StartTransaction() { \
	if (testUtil->startTransaction(__FUNCTION__)) { \
		__FATAL("start transaction failed."); \
		return TestMainStatus::Error; \
	} \
}
#define EndTransaction() { \
	if (testUtil->endTransaction(__FUNCTION__, st)) { \
		__FATAL("end transaction failed."); \
		return TestMainStatus::Error; \
	} \
}

#endif
