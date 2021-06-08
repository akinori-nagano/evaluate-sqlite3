#ifndef __TESTMAIN__TEST_H__
#define __TESTMAIN__TEST_H__

#define ExecKind_Test001_01	(101)
#define ExecKind_Test002_01	(201)
#define ExecKind_Test002_02	(202)
#define ExecKind_Test003_01	(301)
#define ExecKind_Test003_02	(302)

#ifdef __cplusplus
extern "C" {
#endif
	extern int Sqlite3ThreadSafe(void);
    extern int TestInit(const char *dbPath, const char *logDirPath);
    extern int TestExec(const char *dbPath, const char *logDirPath, const char *logTag, int kind, int testCount, long tid);
#ifdef __cplusplus
}
#endif

#endif
