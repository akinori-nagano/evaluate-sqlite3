#ifndef __TESTMAIN__TEST_H__
#define __TESTMAIN__TEST_H__

#ifdef __cplusplus
extern "C" {
#endif
	extern int Sqlite3ThreadSafe(void);
    extern void TestInit(const char *dbPath, const char *logDirPath);
    extern void Test001_01(const char *dbPath, const char *logDirPath, const char *logTag, int testCount, long tid);
    extern void Test002_01(const char *dbPath, const char *logDirPath, const char *logTag, int testCount, long tid);
    extern void Test002_02(const char *dbPath, const char *logDirPath, const char *logTag, int testCount, long tid);

    // extern void TestInit(char *dbPath, char *logDirPath);
    // extern void Test001_01(char *dbPath, char *logDirPath, char *logTag, int testCount, long tid);
    // extern void Test002_01(char *dbPath, char *logDirPath, char *logTag, int testCount, long tid);
    // extern void Test002_02(char *dbPath, char *logDirPath, char *logTag, int testCount, long tid);
#ifdef __cplusplus
}
#endif

#endif
