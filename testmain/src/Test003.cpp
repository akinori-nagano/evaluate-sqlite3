#ifdef FOR_PLATFORM_WINDOWS
#include <windows.h>
#include <sysinfoapi.h>
#endif

#include <iostream>
#include <errno.h>

#include "constants.h"
#include "TestUtility.h"
#include "Test003.h"

static TestMainStatus __Test003_A1(TestUtility *testUtil, pthread_mutex_t *mt, pthread_cond_t *cv, pthread_cond_t *ctDest);
static TestMainStatus __Test003_A2(TestUtility *testUtil, pthread_mutex_t *mt, pthread_cond_t *cv, pthread_cond_t *ctDest);
static TestMainStatus __Test003_B1(TestUtility *testUtil, pthread_mutex_t *mt, pthread_cond_t *cv, pthread_cond_t *ctDest);
static TestMainStatus __Test003_B2(TestUtility *testUtil, pthread_mutex_t *mt, pthread_cond_t *cv, pthread_cond_t *ctDest);
static TestMainStatus __Test003_C1(TestUtility *testUtil, pthread_mutex_t *mt, pthread_cond_t *cv, pthread_cond_t *ctDest);
static TestMainStatus __Test003_C2(TestUtility *testUtil, pthread_mutex_t *mt, pthread_cond_t *cv, pthread_cond_t *ctDest);
static TestMainStatus __pthread_cond_wait(TestUtility *testUtil, pthread_mutex_t *mt, pthread_cond_t *cv);
static TestMainStatus __pthread_cond_signal(TestUtility *testUtil, pthread_cond_t *cv);

#define HaveWaitingPeriod (100)

////////////////////////////////////////////////////////////////////////////////
// EXCLUSIVE
//
//   トランザクション開始からコミット完了までの間、ジャーナルファイルを作らずにデータベースファイルを直接触ります。
//   そのため、その間は他のプロセスは読み込みもできません。
//

TestMainStatus
runTest003_01(TestUtility *testUtil, int testCount, pthread_mutex_t *mt, pthread_cond_t *cv, pthread_cond_t *ctDest)
{
	int res = sqlite3_busy_timeout(testUtil->db, 0);
	if (SQLITE_OK != res) {
		__FATAL("sqlite3_busy_timeout failed. (res=%d)", res);
		return TestMainStatus::Error;
	}

	TestMainStatus st = TestMainStatus::Error;
	for (int i = 0; i < testCount; i++) {
		if ((st = __Test003_A1(testUtil, mt, cv, ctDest))) {
			break;
		}
		if ((st = __Test003_B1(testUtil, mt, cv, ctDest))) {
			break;
		}
		if ((st = __Test003_C1(testUtil, mt, cv, ctDest))) {
			break;
		}
	}
	return st;
}

TestMainStatus
runTest003_02(TestUtility *testUtil, int testCount, pthread_mutex_t *mt, pthread_cond_t *cv, pthread_cond_t *ctDest)
{
	int res = sqlite3_busy_timeout(testUtil->db, 0);
	if (SQLITE_OK != res) {
		__FATAL("sqlite3_busy_timeout failed. (res=%d)", res);
		return TestMainStatus::Error;
	}

	TestMainStatus st = TestMainStatus::Error;
	for (int i = 0; i < testCount; i++) {
		if ((st = __Test003_A2(testUtil, mt, cv, ctDest))) {
			break;
		}
		if ((st = __Test003_B2(testUtil, mt, cv, ctDest))) {
			break;
		}
		if ((st = __Test003_C2(testUtil, mt, cv, ctDest))) {
			break;
		}
	}
	return st;
}

TestMainStatus
__Test003_A1(TestUtility *testUtil, pthread_mutex_t *mt, pthread_cond_t *cv, pthread_cond_t *ctDest)
{
	TestMainStatus st = TestMainStatus::Ok;

	__INFO("#### START");
	// 全データ削除
	testUtil->deleteAll();
	// 初期データ
	{
		Table001 *p = GetTest001Data(1, Table001Kind::Insert);
		if ((st = testUtil->insertTable001(p)) != TestMainStatus::Ok) {
			__FATAL("insert failed. %d.", st);
			goto END;
		}
	}

	// Step1
	//    EXCLUSIVE tansactionの実行

	__INFO("#### STEP1: start");
	StartTransaction(SQLITE3_EXCLUSIVE);
	myMicroSleep(HaveWaitingPeriod);

	__INFO("wakeup for 02.");
	if (__pthread_cond_signal(testUtil, ctDest)) {
		return TestMainStatus::Error;
	}

	// 02の操作完了まで待つ
	__INFO("waiting...");
	if (__pthread_cond_wait(testUtil, mt, cv)) {
		return TestMainStatus::Error;
	}
	myMicroSleep(HaveWaitingPeriod);

	EndTransaction();

	__INFO("finished.");
	return TestMainStatus::Ok;

END:
	st = TestMainStatus::Error;
	EndTransaction();
	__FATAL("error end.");
	return TestMainStatus::Error;
}

TestMainStatus
__Test003_A2(TestUtility *testUtil, pthread_mutex_t *mt, pthread_cond_t *cv, pthread_cond_t *ctDest)
{
	TestMainStatus st = TestMainStatus::Ok;

	__INFO("#### START");

	if (__pthread_cond_wait(testUtil, mt, cv)) {
		return TestMainStatus::Error;
	}
	myMicroSleep(HaveWaitingPeriod);

	// Step2
	//    IMMEDIATE tansactionが失敗すること

	myMicroSleep(500);
	__INFO("#### STEP2: IMMEDIATE select");

	if ((st = testUtil->startTransaction(__FUNCTION__, SQLITE3_IMMEDIATE)) == TestMainStatus::Ok) {
		__FATAL("bad access. IMMEDIATE success. %d.", st);
		return TestMainStatus::Error;
	}

	// Step3
	//    DEFERRED tansaction
	//    select、insert に失敗すること

	myMicroSleep(500);
	__INFO("#### STEP3: DEFERRED select");

	StartTransaction(SQLITE3_DEFERRED);
	{
		Table001 t;
		if ((st = testUtil->selectTable001(1, &t)) == TestMainStatus::Ok) {
			__FATAL("bad access. select success. %d.", st);
			goto END;
		}
	}
	EndTransaction();

	StartTransaction(SQLITE3_DEFERRED);
	{
		Table001 *p = GetTest001Data(1, Table001Kind::Update1);
		if ((st = testUtil->updateTable001(p)) == TestMainStatus::Ok) {
			__FATAL("bad access. update success. %d.", st);
			goto END;
		}
	}
	EndTransaction();

	__INFO("wakeup for 01.");
	if (__pthread_cond_signal(testUtil, ctDest)) {
		return TestMainStatus::Error;
	}

	__INFO("finished.");
	return TestMainStatus::Ok;

END:
	st = TestMainStatus::Error;
	EndTransaction();
	__FATAL("error end.");
	return TestMainStatus::Error;
}

////////////////////////////////////////////////////////////////////////////////
// IMMEDIATE
//
//   transactionが開始されるとすぐに予約ロックされる。
//   その間は、書き込みがロックされると共に、別の Immediate transactionも禁止される。
//   しかし、読み込みはできる。
//

TestMainStatus
__Test003_B1(TestUtility *testUtil, pthread_mutex_t *mt, pthread_cond_t *cv, pthread_cond_t *ctDest)
{
	TestMainStatus st = TestMainStatus::Ok;

	__INFO("#### START");
	// 全データ削除
	testUtil->deleteAll();
	// 初期データ
	{
		Table001 *p = GetTest001Data(1, Table001Kind::Insert);
		st = testUtil->insertTable001(p);
		if (st != TestMainStatus::Ok) {
			__FATAL("insert failed. %d.", st);
			goto END;
		}
	}

	// Step1
	//    IMMEDIATE tansactionの実行

	__INFO("#### STEP1: start");
	StartTransaction(SQLITE3_IMMEDIATE);
	myMicroSleep(HaveWaitingPeriod);

	__INFO("wakeup for 02.");
	if (__pthread_cond_signal(testUtil, ctDest)) {
		return TestMainStatus::Error;
	}

	// 02の操作完了まで待つ
	__INFO("waiting...");
	if (__pthread_cond_wait(testUtil, mt, cv)) {
		return TestMainStatus::Error;
	}
	myMicroSleep(HaveWaitingPeriod);

	// Step4
	// 書き込み動作
	__INFO("#### STEP4: start");

	{
		Table001 *p = GetTest001Data(1, Table001Kind::Update1);
		if ((st = testUtil->updateTable001(p)) != TestMainStatus::Ok) {
			__FATAL("update failed. %d.", st);
			goto END;
		}
	}

	// 02の操作完了まで待つ
	__INFO("waiting...");
	if (__pthread_cond_wait(testUtil, mt, cv)) {
		return TestMainStatus::Error;
	}
	myMicroSleep(HaveWaitingPeriod);

	EndTransaction();

	__INFO("finished.");
	return TestMainStatus::Ok;

END:
	st = TestMainStatus::Error;
	EndTransaction();
	__FATAL("error end.");
	return st;
}

TestMainStatus
__Test003_B2(TestUtility *testUtil, pthread_mutex_t *mt, pthread_cond_t *cv, pthread_cond_t *ctDest)
{
	TestMainStatus st = TestMainStatus::Ok;

	__INFO("#### START");

	if (__pthread_cond_wait(testUtil, mt, cv)) {
		return TestMainStatus::Error;
	}
	myMicroSleep(HaveWaitingPeriod);

	// Step2
	//    IMMEDIATE tansactionが失敗すること

	myMicroSleep(500);
	__INFO("#### STEP2: IMMEDIATE select");

	if ((st = testUtil->startTransaction(__FUNCTION__, SQLITE3_IMMEDIATE)) == TestMainStatus::Ok) {
		__FATAL("bad access. IMMEDIATE success. %d.", st);
		return TestMainStatus::Error;
	}

	// Step3
	//    DEFERRED tansactionが失敗すること
	//    selectに成功、updateに失敗すること

	__INFO("#### STEP3: start");

	StartTransaction(SQLITE3_DEFERRED);
	{
		Table001 t;
		if ((st = testUtil->selectTable001(1, &t)) != TestMainStatus::Ok) {
			__FATAL("select failed. %d.", st);
			goto END;
		}
	}
	EndTransaction();

	StartTransaction(SQLITE3_DEFERRED);
	{
		Table001 *p = GetTest001Data(1, Table001Kind::Update1);
		if ((st = testUtil->updateTable001(p)) == TestMainStatus::Ok) {
			__FATAL("bad access. update success. %d.", st);
			goto END;
		}
	}
	EndTransaction();

	__INFO("wakeup for 01.");
	if (__pthread_cond_signal(testUtil, ctDest)) {
		return TestMainStatus::Error;
	}

	// Step5
	//    DEFERRED tansaction
	//    selectに成功、updateに失敗すること

	myMicroSleep(500);
	__INFO("#### STEP5: DEFERRED select, insert");

	StartTransaction(SQLITE3_DEFERRED);
	{
		Table001 t;
		if ((st = testUtil->selectTable001(1, &t))) {
			__FATAL("bad access. select failed. %d.", st);
			goto END;
		}
		// updateされたデータではなく初期データが取得できること
		Table001 *p = GetTest001Data(1, Table001Kind::Insert);
		if (testUtil->diffTable001(&t, p)) {
			__FATAL("bad access. invalid select.");
			st = TestMainStatus::Error;
			goto END;
		}
	}
	EndTransaction();

	StartTransaction(SQLITE3_DEFERRED);
	{
		Table001 *p = GetTest001Data(1, Table001Kind::Update1);
		if ((st = testUtil->updateTable001(p)) == TestMainStatus::Ok) {
			__FATAL("bad access. update success. %d.", st);
			goto END;
		}
	}
	EndTransaction();

	__INFO("wakeup for 01.");
	if (__pthread_cond_signal(testUtil, ctDest)) {
		return TestMainStatus::Error;
	}

	__INFO("finished.");
	return TestMainStatus::Ok;

END:
	st = TestMainStatus::Error;
	EndTransaction();
	__FATAL("error end.");
	return st;
}

////////////////////////////////////////////////////////////////////////////////
// DEFERRED
//
//   トランザクション開始時にはロックを取得せず, データの読み込み/書き込みをする時点までロック取得を延期する
//   そのため, BEGINステートメントによるトランザクション開始のみでは何のロックも取得されない
//   ロックの取得がBEGIN～データの読み込み/書き込みまで延期されるため, 別トランザクションによるデータ書き込みの
// 割り込みが発生する可能性がある. (ANSI/ISO SQL標準では READ_COMMITTED に相当. )
//

TestMainStatus
__Test003_C1(TestUtility *testUtil, pthread_mutex_t *mt, pthread_cond_t *cv, pthread_cond_t *ctDest)
{
	TestMainStatus st = TestMainStatus::Ok;

	__INFO("#### START");
	// 全データ削除
	testUtil->deleteAll();
	// 初期データ
	{
		Table001 *p = GetTest001Data(1, Table001Kind::Insert);
		st = testUtil->insertTable001(p);
		if (st != TestMainStatus::Ok) {
			__FATAL("insert failed. %d.", st);
			goto END;
		}
	}

	// Step1
	//    DEFERRED tansactionの実行

	__INFO("#### STEP1: start");
	StartTransaction(SQLITE3_DEFERRED);
	myMicroSleep(HaveWaitingPeriod);

	__INFO("wakeup for 02.");
	if (__pthread_cond_signal(testUtil, ctDest)) {
		return TestMainStatus::Error;
	}

	// 02の操作完了まで待つ
	__INFO("waiting...");
	if (__pthread_cond_wait(testUtil, mt, cv)) {
		return TestMainStatus::Error;
	}
	myMicroSleep(HaveWaitingPeriod);

	// Step3
	// 書き込み動作
	__INFO("#### STEP3: start");

	{
		Table001 *p = GetTest001Data(1, Table001Kind::Update1);
		if ((st = testUtil->updateTable001(p)) != TestMainStatus::Ok) {
			__FATAL("update failed. %d.", st);
			goto END;
		}
	}

	__INFO("wakeup for 02.");
	if (__pthread_cond_signal(testUtil, ctDest)) {
		return TestMainStatus::Error;
	}

	__INFO("waiting...");
	if (__pthread_cond_wait(testUtil, mt, cv)) {
		return TestMainStatus::Error;
	}
	myMicroSleep(HaveWaitingPeriod);

	EndTransaction();

	__INFO("finished.");
	return TestMainStatus::Ok;

END:
	st = TestMainStatus::Error;
	EndTransaction();
	__FATAL("error end.");
	return st;
}

TestMainStatus
__Test003_C2(TestUtility *testUtil, pthread_mutex_t *mt, pthread_cond_t *cv, pthread_cond_t *ctDest)
{
	TestMainStatus st = TestMainStatus::Ok;

	__INFO("#### START");

	if (__pthread_cond_wait(testUtil, mt, cv)) {
		return TestMainStatus::Error;
	}
	myMicroSleep(HaveWaitingPeriod);

	// Step2
	//    DEFERRED tansaction
	//    selectに成功すること
	__INFO("#### STEP2: start");

	StartTransaction(SQLITE3_DEFERRED);

	{
		Table001 t;
		if ((st = testUtil->selectTable001(1, &t)) != TestMainStatus::Ok) {
			__FATAL("select failed. %d.", st);
			goto END;
		}
	}

	__INFO("wakeup for 01.");
	if (__pthread_cond_signal(testUtil, ctDest)) {
		return TestMainStatus::Error;
	}

	__INFO("waiting...");
	if (__pthread_cond_wait(testUtil, mt, cv)) {
		return TestMainStatus::Error;
	}

	// Step4
	//    DEFERRED tansaction
	//    selectに成功、updateに失敗すること

	__INFO("#### STEP4: start");

	{
		Table001 t;
		if ((st = testUtil->selectTable001(1, &t)) != TestMainStatus::Ok) {
			__FATAL("select failed. %d.", st);
			goto END;
		}
		// updateされたデータではなく初期データが取得できること
		Table001 *p = GetTest001Data(1, Table001Kind::Insert);
		if (testUtil->diffTable001(&t, p)) {
			__FATAL("bad access. invalid select.");
			st = TestMainStatus::Error;
			goto END;
		}
	}
	{
		Table001 *p = GetTest001Data(1, Table001Kind::Update1);
		if ((st = testUtil->updateTable001(p)) == TestMainStatus::Ok) {
			__FATAL("bad access. update success. %d.", st);
			st = TestMainStatus::Error;
			goto END;
		}
	}

	EndTransaction();

	__INFO("wakeup for 01.");
	if (__pthread_cond_signal(testUtil, ctDest)) {
		return TestMainStatus::Error;
	}

	__INFO("finished.");
	return TestMainStatus::Ok;

END:
	st = TestMainStatus::Error;
	EndTransaction();
	__FATAL("error end.");
	return st;
}

////////////////////////////////////////////////////////////////////////////////
//

TestMainStatus
__pthread_cond_wait(TestUtility *testUtil, pthread_mutex_t *mt, pthread_cond_t *cv)
{
	int ret = ::pthread_cond_wait(cv, mt);
	if (ret) {
		__FATAL("failed. (ret=%d)(errno=%d)", ret, errno);
		return TestMainStatus::Error;
	}
	return TestMainStatus::Ok;
}

TestMainStatus
__pthread_cond_signal(TestUtility *testUtil, pthread_cond_t *cv)
{
	int ret = ::pthread_cond_signal(cv);
	if (ret) {
		__FATAL("failed. (ret=%d)(errno=%d)", ret, errno);
		return TestMainStatus::Error;
	}
	return TestMainStatus::Ok;
}
