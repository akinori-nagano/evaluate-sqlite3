#include <stdio.h>
#include <pthread.h>
#include "runner.h"
#include "Test.h"

#define dbPath	"../testmain/db.sqlite3"
#define logDirPath	"./logs/"


static void * test003_01(void *arg);
static void * test003_02(void *arg);


static const int Test003Count = 10;


Runner::Runner()
{
	printf("%s\n", __PRETTY_FUNCTION__);
}

void
Runner::init(void)
{
	TestInit(dbPath, logDirPath);
}

void
Runner::runTest003(void)
{
	pthread_t trd_id[2];

    pthread_create(&trd_id[0], NULL, test003_01, (void *)NULL);
    pthread_create(&trd_id[1], NULL, test003_02, (void *)NULL);

	const int trd_num = sizeof(trd_id) / sizeof(pthread_t);
	for (int i = 0; i < trd_num; i++) {
		pthread_join(trd_id[i], NULL);
	}
}

void *
test003_01(void *)
{
	int ret = TestExec(dbPath, logDirPath, "test003_01", 301, Test003Count, 1);
	printf("%s: finished. ret is %d\n", __PRETTY_FUNCTION__, ret);
	return ((void *)NULL);
}

void *
test003_02(void *)
{
	int ret = TestExec(dbPath, logDirPath, "test003_02", 302, Test003Count, 2);
	printf("%s: finished. ret is %d\n", __PRETTY_FUNCTION__, ret);
	return ((void *)NULL);
}
