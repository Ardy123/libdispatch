#define _GNU_SOURCE
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <CUnit/CUnit.h>
#include <CUnit/Automated.h>
#include <CUnit/Basic.h>
#include "dispatch.h"
/* ------------ */
#define OUTPUT_FILE "unittest"
/* ------------ */
typedef struct {
  int m_run;
  int m_end;
} test_context;
/* ------------ */
static int _runTests(const char *);
static void _paramTest_01(void);
static void _paramTest_02(void);
static void _paramTest_03(void);
static void _paramTest_04(void);
static void _paramTest_05(void);
static void _paramTest_06(void);
static void _paramTest_07(void);
static void _func_test_setUp(void);
static void _func_test_tearDown(void);
static void _funcTest_01(void);
static void _funcTest_02(void);
static void _funcTest_03(void);
static void _funcTest_04(void);
static void _funcTest_05(void);
static void _funcTest_06(void);
static void _funcTest_07(void);
static void _funcTest_08(void);
static void _funcTest_09(void);
static void _funcTest_10(void);
static void _funcTest_11(void);
static void _funcTest_12(void);
static void _funcTest_13(void);
static void _funcTest_14(void);
static void _funcTest_15(void);
static void _funcTest_16(void);
static void _funcTest_17(void);
static void * _taskRun(dispatch_task, void *);
static void _taskFin(dispatch_task, void *, void *);
static void _taskCnl(dispatch_task, void *);
static void * _taskRunReschedule(dispatch_task, void *);
static void _taskFinReschedule(dispatch_task, void *, void *);
static void * _taskRunWait(dispatch_task, void *);
static void _taskFinWait(dispatch_task, void *, void *);
/* ------------ */
static CU_TestInfo param_tests[] = {
  { "Paramater Test 01", _paramTest_01 },
  { "Paramater Test 02", _paramTest_02 },
  { "Paramater Test 03", _paramTest_03 },
  { "Paramater Test 04", _paramTest_04 },
  { "Paramater Test 05", _paramTest_05 },
  { "Paramater Test 06", _paramTest_06 },
  { "Paramater Test 07", _paramTest_07 },  
  CU_TEST_INFO_NULL
};
/* ------------ */
static CU_TestInfo funct_tests[] = {
  { "Functionality Test 01", _funcTest_01 },
  { "Functionality Test 02", _funcTest_02 },
  { "Functionality Test 03", _funcTest_03 },
  { "Functionality Test 04", _funcTest_04 },
  { "Functionality Test 05", _funcTest_05 },
  { "Functionality Test 06", _funcTest_06 },
  { "Functionality Test 07", _funcTest_07 },
  { "Functionality Test 08", _funcTest_08 },
  { "Functionality Test 09", _funcTest_09 },
  { "Functionality Test 10", _funcTest_10 },
  { "Functionality Test 11", _funcTest_11 },
  { "Functionality Test 12", _funcTest_12 },
  { "Functionality Test 13", _funcTest_13 },
  { "Functionality Test 14", _funcTest_14 },
  { "Functionality Test 15", _funcTest_15 },
  { "Functionality Test 16", _funcTest_16 },
  { "Functionality Test 17", _funcTest_17 },  
  CU_TEST_INFO_NULL
};
/* ------------ */
static int gCanceled;
static pthread_mutex_t gCancelLock;
static pthread_cond_t gCancelCond;
/* ------------ */
static CU_SuiteInfo suites[] = {
  {
    "Paramater Tests",
    NULL,
    NULL,
    NULL,
    NULL,
    param_tests
  },
  {
    "Functionality Tests",
    NULL,
    NULL,
    _func_test_setUp,
    _func_test_tearDown,
    funct_tests
  },
  CU_SUITE_INFO_NULL
};
/* ------------ */

static void
_paramTest_01(void) {
  int testBuffer[4];
  CU_ASSERT(NULL == dispatch_init(NULL, NULL, NULL));
  CU_ASSERT(NULL == dispatch_init(NULL, NULL, &testBuffer[0]));
  CU_ASSERT(NULL == dispatch_init(NULL, _taskCnl, NULL));
  CU_ASSERT(NULL == dispatch_init(NULL, _taskCnl, &testBuffer[0]));  
}
/* ------------ */
static void
_paramTest_02(void) {
  int testBuffer[4];
  CU_ASSERT(NULL == dispatch_initEx(NULL, NULL, NULL, NULL));
  CU_ASSERT(NULL == dispatch_initEx(NULL, NULL, NULL, &testBuffer[0]));
  CU_ASSERT(NULL == dispatch_initEx(NULL, NULL, _taskFin, NULL));
  CU_ASSERT(NULL == dispatch_initEx(NULL, NULL, _taskFin, &testBuffer[0]));  
  CU_ASSERT(NULL == dispatch_initEx(NULL, _taskCnl, _taskFin, &testBuffer[0]));
  CU_ASSERT(NULL == dispatch_initEx(NULL, _taskCnl, NULL, &testBuffer[0]));
  CU_ASSERT(NULL == dispatch_initEx(NULL, _taskCnl, NULL, NULL));    
  CU_ASSERT(NULL == dispatch_initEx(_taskRun, NULL, NULL, NULL));
  CU_ASSERT(NULL == dispatch_initEx(_taskRun, NULL, NULL, &testBuffer[0]));
  CU_ASSERT(NULL == dispatch_initEx(_taskRun, _taskCnl, NULL, &testBuffer[0]));
  CU_ASSERT(NULL == dispatch_initEx(_taskRun, NULL, _taskFin, &testBuffer[0]));
}
/* ------------ */
static void
_paramTest_03(void) {
  CU_ASSERT(DISPATCH_INVALID_OPERAND == dispatch_destroy(NULL));
}
/* ------------ */
static void
_paramTest_04(void) {
  CU_ASSERT(DISPATCH_INVALID_OPERAND == dispatch_schedule(NULL));
}
/* ------------ */
static void
_paramTest_05(void) {
  CU_ASSERT(DISPATCH_INVALID_OPERAND == dispatch_cancel(NULL));
}
/* ------------ */
static void
_paramTest_06(void) {
  dispatch_task testTasks[1];
  CU_ASSERT(DISPATCH_INVALID_OPERAND == dispatch_wait(NULL, 0));
  CU_ASSERT(DISPATCH_INVALID_OPERAND == dispatch_wait(NULL, 5));
  CU_ASSERT(DISPATCH_INVALID_OPERAND == dispatch_wait(testTasks, 0));
}
/* ------------ */
static void
_paramTest_07(void) {
  CU_ASSERT(NULL == dispatch_return(NULL));
}
/* ------------ */
static void
_func_test_setUp(void) {
  gCanceled = 0;
  pthread_mutex_init(&gCancelLock, NULL);
  pthread_cond_init(&gCancelCond, NULL);  
}
/* ------------ */
static void
_func_test_tearDown(void) {
  pthread_mutex_destroy(&gCancelLock);
  pthread_cond_destroy(&gCancelCond);  
}
/* ------------ */
static void
_funcTest_01(void) {
  CU_ASSERT(DISPATCH_OK == dispatch_engine_init());
  CU_ASSERT(DISPATCH_OK == dispatch_engine_destroy());
}
/* ------------ */
static void
_funcTest_02(void) {
  dispatch_task pTask;
  CU_ASSERT(DISPATCH_OK == dispatch_engine_init());
  CU_ASSERT(NULL != (pTask = dispatch_init(_taskRun, _taskCnl, NULL)));
  CU_ASSERT(DISPATCH_OK == dispatch_destroy(pTask));
  CU_ASSERT(DISPATCH_OK == dispatch_engine_destroy());
}
/* ------------ */
static void
_funcTest_03(void) {
  dispatch_task pTask;
  CU_ASSERT(DISPATCH_OK == dispatch_engine_init());
  CU_ASSERT(NULL != (pTask = dispatch_initEx(_taskRun,
					     _taskCnl,
					     _taskFin,
					     NULL)));
  CU_ASSERT(DISPATCH_OK == dispatch_destroy(pTask));
  CU_ASSERT(DISPATCH_OK == dispatch_engine_destroy());
}
/* ------------ */
static void
_funcTest_04(void) {
  dispatch_task pTask;
  CU_ASSERT(DISPATCH_OK == dispatch_engine_init());
  CU_ASSERT(NULL != (pTask = dispatch_init(_taskRun, _taskCnl, NULL)));
  CU_ASSERT(DISPATCH_OK == dispatch_schedule(pTask));
  CU_ASSERT(DISPATCH_OK == dispatch_wait(&pTask, 1));
  CU_ASSERT(DISPATCH_OK == dispatch_destroy(pTask));
  CU_ASSERT(DISPATCH_OK == dispatch_engine_destroy());
}
/* ------------ */
static void
_funcTest_05(void) {
  dispatch_task pTask;
  CU_ASSERT(DISPATCH_OK == dispatch_engine_init());
  CU_ASSERT(NULL != (pTask = dispatch_initEx(_taskRun,
					     _taskCnl,
					     _taskFin,
					     NULL)));
  CU_ASSERT(DISPATCH_OK == dispatch_schedule(pTask));
  CU_ASSERT(DISPATCH_OK == dispatch_wait(&pTask, 1));
  CU_ASSERT(DISPATCH_OK == dispatch_destroy(pTask));
  CU_ASSERT(DISPATCH_OK == dispatch_engine_destroy());
}
/* ------------ */
static void
_funcTest_06(void) {
  dispatch_task pTask;
  test_context ctx = { 0, 0 };
  CU_ASSERT(DISPATCH_OK == dispatch_engine_init());
  CU_ASSERT(NULL != (pTask = dispatch_init(_taskRun, _taskCnl, &ctx)));
  CU_ASSERT(DISPATCH_OK == dispatch_schedule(pTask));
  CU_ASSERT(DISPATCH_OK == dispatch_wait(&pTask, 1));
  CU_ASSERT(&ctx == dispatch_return(pTask));
  CU_ASSERT(1 == ctx.m_run);
  CU_ASSERT(DISPATCH_OK == dispatch_destroy(pTask));
  CU_ASSERT(DISPATCH_OK == dispatch_engine_destroy());
}
/* ------------ */
static void
_funcTest_07(void) {
  dispatch_task pTask;
  test_context ctx = { 0, 0 };
  CU_ASSERT(DISPATCH_OK == dispatch_engine_init());
  CU_ASSERT(NULL != (pTask = dispatch_initEx(_taskRun,
					     _taskCnl,
					     _taskFin,
					     &ctx)));
  CU_ASSERT(DISPATCH_OK == dispatch_schedule(pTask));
  CU_ASSERT(DISPATCH_OK == dispatch_wait(&pTask, 1));
  CU_ASSERT(&ctx == dispatch_return(pTask));
  CU_ASSERT(1 == ctx.m_run);
  CU_ASSERT(1 == ctx.m_end);
  CU_ASSERT(DISPATCH_OK == dispatch_destroy(pTask));
  CU_ASSERT(DISPATCH_OK == dispatch_engine_destroy());
}
/* ------------ */
static void
_funcTest_08(void) {
  int ndx;
  dispatch_task * ppTask;
  test_context * pCtx;
  const int nTasks = (int)sysconf(_SC_NPROCESSORS_ONLN) * 2;
  /* allocate memory for test */
  ppTask = malloc(sizeof(dispatch_task) * nTasks);
  pCtx = malloc(sizeof(test_context) * nTasks);
  memset(pCtx, 0, sizeof(test_context) * nTasks);
  /* run tests */
  CU_ASSERT(DISPATCH_OK == dispatch_engine_init());
  for (ndx = 0; ndx < nTasks; ++ndx) {
    CU_ASSERT(NULL != (ppTask[ndx] = dispatch_init(_taskRun,
						   _taskCnl,
						   &pCtx[ndx])));
    CU_ASSERT(DISPATCH_OK == dispatch_schedule(ppTask[ndx]));
  }
  CU_ASSERT(DISPATCH_OK == dispatch_wait(ppTask, nTasks));
  for (ndx = 0; ndx < nTasks; ++ndx) {
    CU_ASSERT(&pCtx[ndx] == dispatch_return(ppTask[ndx]));
    CU_ASSERT(1 == pCtx[ndx].m_run);
    CU_ASSERT(DISPATCH_OK == dispatch_destroy(ppTask[ndx]));
  }
  CU_ASSERT(DISPATCH_OK == dispatch_engine_destroy());
  /* free test memory */
  free(ppTask);
  free(pCtx);
}
/* ------------ */
static void
_funcTest_09(void) {
  int ndx;
  dispatch_task * ppTask;
  test_context * pCtx;
  const int nTasks = (int)sysconf(_SC_NPROCESSORS_ONLN) * 2;
  /* allocate memory for test */
  ppTask = malloc(sizeof(dispatch_task) * nTasks);
  pCtx = malloc(sizeof(test_context) * nTasks);
  memset(pCtx, 0, sizeof(test_context) * nTasks);
  /* run tests */
  CU_ASSERT(DISPATCH_OK == dispatch_engine_init());
  for (ndx = 0; ndx < nTasks; ++ndx) {
    CU_ASSERT(NULL != (ppTask[ndx] =
		       dispatch_initEx(_taskRun,
				       _taskCnl,
				       _taskFin, &pCtx[ndx])));
    CU_ASSERT(DISPATCH_OK == dispatch_schedule(ppTask[ndx]));
  }
  CU_ASSERT(DISPATCH_OK == dispatch_wait(ppTask, nTasks));
  for (ndx = 0; ndx < nTasks; ++ndx) {
    CU_ASSERT(&pCtx[ndx] == dispatch_return(ppTask[ndx]));
    CU_ASSERT(1 == pCtx[ndx].m_run);
    CU_ASSERT(1 == pCtx[ndx].m_end);
    CU_ASSERT(DISPATCH_OK == dispatch_destroy(ppTask[ndx]));
  }
  CU_ASSERT(DISPATCH_OK == dispatch_engine_destroy());
  /* free test memory */
  free(ppTask);
  free(pCtx);
}
/* ------------ */
static void
_funcTest_10(void) {
  dispatch_task pTask;
  test_context ctx = { 0, 0 };
  CU_ASSERT(DISPATCH_OK == dispatch_engine_init());
  CU_ASSERT(NULL != (pTask = dispatch_init(_taskRun, _taskCnl, &ctx)));
  CU_ASSERT(DISPATCH_OK == dispatch_schedule(pTask));
  CU_ASSERT(DISPATCH_OK == dispatch_wait(&pTask, 1));
  CU_ASSERT(&ctx == dispatch_return(pTask));
  CU_ASSERT(1 == ctx.m_run);
  CU_ASSERT(DISPATCH_OK == dispatch_schedule(pTask));
  CU_ASSERT(DISPATCH_OK == dispatch_wait(&pTask, 1));
  CU_ASSERT(&ctx == dispatch_return(pTask));
  CU_ASSERT(2 == ctx.m_run);  
  CU_ASSERT(DISPATCH_OK == dispatch_destroy(pTask));
  CU_ASSERT(DISPATCH_OK == dispatch_engine_destroy());
}
/* ------------ */
static void
_funcTest_11(void) {
  dispatch_task pTask;
  test_context ctx = { 0, 0 };
  CU_ASSERT(DISPATCH_OK == dispatch_engine_init());
  CU_ASSERT(NULL != (pTask = dispatch_initEx(_taskRun,
					     _taskCnl,
					     _taskFin,
					     &ctx)));
  CU_ASSERT(DISPATCH_OK == dispatch_schedule(pTask));
  CU_ASSERT(DISPATCH_OK == dispatch_wait(&pTask, 1));
  CU_ASSERT(&ctx == dispatch_return(pTask));
  CU_ASSERT(1 == ctx.m_run);
  CU_ASSERT(1 == ctx.m_end);
  CU_ASSERT(DISPATCH_OK == dispatch_schedule(pTask));
  CU_ASSERT(DISPATCH_OK == dispatch_wait(&pTask, 1));
  CU_ASSERT(&ctx == dispatch_return(pTask));
  CU_ASSERT(2 == ctx.m_run);
  CU_ASSERT(2 == ctx.m_end);
  CU_ASSERT(DISPATCH_OK == dispatch_destroy(pTask));
  CU_ASSERT(DISPATCH_OK == dispatch_engine_destroy());
}
/* ------------ */
static void
_funcTest_12(void) {
  dispatch_task pTask;
  test_context ctx = { 0, 0 };
  CU_ASSERT(DISPATCH_OK == dispatch_engine_init());
  CU_ASSERT(NULL != (pTask = dispatch_initEx(_taskRunReschedule,
					     _taskCnl,
					     _taskFinReschedule,
					     &ctx)));
  CU_ASSERT(DISPATCH_OK == dispatch_schedule(pTask));
  CU_ASSERT(DISPATCH_OK == dispatch_wait(&pTask, 1));
  CU_ASSERT(&ctx == dispatch_return(pTask));
  CU_ASSERT(1 == ctx.m_run);
  CU_ASSERT(1 == ctx.m_end);
  CU_ASSERT(DISPATCH_OK == dispatch_destroy(pTask));
  CU_ASSERT(DISPATCH_OK == dispatch_engine_destroy());
}
/* ------------ */
static void
_funcTest_13(void) {
  dispatch_task pTask;
  test_context ctx = { 0, 0 };
  CU_ASSERT(DISPATCH_OK == dispatch_engine_init());
  CU_ASSERT(NULL != (pTask = dispatch_initEx(_taskRunWait,
					     _taskCnl,
					     _taskFinWait,
					     &ctx)));
  CU_ASSERT(DISPATCH_OK == dispatch_schedule(pTask));
  CU_ASSERT(DISPATCH_OK == dispatch_wait(&pTask, 1));
  CU_ASSERT(&ctx == dispatch_return(pTask));
  CU_ASSERT(1 == ctx.m_run);
  CU_ASSERT(1 == ctx.m_end);
  CU_ASSERT(DISPATCH_OK == dispatch_destroy(pTask));
  CU_ASSERT(DISPATCH_OK == dispatch_engine_destroy());
}
/* ------------ */
static void
_funcTest_14(void) {
  dispatch_task pTask;
  test_context ctx = { 0, 0 };
  CU_ASSERT(DISPATCH_OK == dispatch_engine_init());
  CU_ASSERT(NULL != (pTask = dispatch_initEx(_taskRun,
					     _taskCnl,
					     _taskFin,
					     &ctx)));
  CU_ASSERT(DISPATCH_OK == dispatch_schedule(pTask));
  pthread_yield();
  dispatch_cancel(pTask);
  pthread_yield();  
  CU_ASSERT(DISPATCH_OK == dispatch_wait(&pTask, 1));
  pthread_yield();
  pthread_mutex_lock(&gCancelLock);
  while (!gCanceled && !ctx.m_end) {
    pthread_cond_wait(&gCancelCond, &gCancelLock);
  }
  pthread_mutex_unlock(&gCancelLock); 
  CU_ASSERT(DISPATCH_OK == dispatch_destroy(pTask));  
  CU_ASSERT(DISPATCH_OK == dispatch_engine_destroy());
}
/* ------------ */
static void
_funcTest_15(void) {
  dispatch_task pTask;
  test_context ctx = { 0, 0 };
  CU_ASSERT(DISPATCH_OK == dispatch_engine_init());
  CU_ASSERT(NULL != (pTask = dispatch_init(_taskRun, _taskCnl, &ctx)));
  CU_ASSERT(DISPATCH_INVALID_OPERATION == dispatch_cancel(pTask));
  CU_ASSERT(DISPATCH_OK == dispatch_destroy(pTask));
  CU_ASSERT(DISPATCH_OK == dispatch_engine_destroy());
}
/* ------------ */
static void
_funcTest_16(void) {
  dispatch_task pTask;
  test_context ctx = { 0, 0 };
  CU_ASSERT(DISPATCH_OK == dispatch_engine_init());
  CU_ASSERT(NULL != (pTask = dispatch_init(_taskRun, _taskCnl, &ctx)));
  CU_ASSERT(DISPATCH_OK == dispatch_schedule(pTask));
  CU_ASSERT(DISPATCH_OK == dispatch_wait(&pTask, 1));
  CU_ASSERT(DISPATCH_INVALID_OPERATION == dispatch_cancel(pTask));  
  CU_ASSERT(DISPATCH_OK == dispatch_destroy(pTask));
  CU_ASSERT(DISPATCH_OK == dispatch_engine_destroy());
}
/* ------------ */
static void
_funcTest_17(void) {
  dispatch_task pTask;
  test_context ctx = { 0, 0 };
  CU_ASSERT(DISPATCH_OK == dispatch_engine_init());
  CU_ASSERT(NULL != (pTask = dispatch_init(_taskRun, _taskCnl, &ctx)));
  CU_ASSERT(DISPATCH_OK == dispatch_schedule(pTask));
  CU_ASSERT(DISPATCH_INVALID_OPERATION == dispatch_destroy(pTask));  
  CU_ASSERT(DISPATCH_OK == dispatch_wait(&pTask, 1));
  CU_ASSERT(DISPATCH_OK == dispatch_destroy(pTask));
  CU_ASSERT(DISPATCH_OK == dispatch_engine_destroy());
}
/* ------------ */
static void *
_taskRun(dispatch_task pTask, void * pContext) {
  test_context * pCtx = (test_context *)pContext;
  if (pCtx) {
    pCtx->m_run++;
  }    
  return pCtx;
}
/* ------------ */
static void
_taskFin(const dispatch_task pTask, void * pContext, void * pReturn) {
  test_context * pCtx = (test_context *)pContext;
  if (pCtx) {
    pCtx->m_end++;
  }
  pthread_mutex_lock(&gCancelLock);
  pthread_cond_signal(&gCancelCond);
  pthread_mutex_unlock(&gCancelLock);  
}
/* ------------ */
static void
_taskCnl(const dispatch_task pTask, void * pContext) {
  pthread_mutex_lock(&gCancelLock);
  gCanceled++;
  pthread_cond_signal(&gCancelCond);
  pthread_mutex_unlock(&gCancelLock);
}
/* ------------ */
static void *
_taskRunReschedule(dispatch_task pTask, void * pContext) {
  test_context * pCtx = (test_context *)pContext;
  pCtx->m_run++;
  CU_ASSERT(DISPATCH_INVALID_OPERATION == dispatch_schedule(pTask));
  return pCtx;
}
/* ------------ */
static void
_taskFinReschedule(dispatch_task pTask, void * pContext, void * pReturn) {
  test_context * pCtx = (test_context *)pContext;
  pCtx->m_end++;
  CU_ASSERT(DISPATCH_INVALID_OPERATION == dispatch_schedule(pTask));
}
/* ------------ */
static void *
_taskRunWait(dispatch_task pTask, void * pContext) {
  test_context * pCtx = (test_context *)pContext;
  pCtx->m_run++;
  CU_ASSERT(DISPATCH_INVALID_OPERATION == dispatch_wait(&pTask, 1));
  return pCtx;
}
/* ------------ */
static void
_taskFinWait(dispatch_task pTask, void * pContext, void * pReturn) {
  test_context * pCtx = (test_context *)pContext;
  pCtx->m_end++;
  CU_ASSERT(DISPATCH_INVALID_OPERATION == dispatch_wait(&pTask, 1));
}
/* ------------ */
static int
_runTests(const char * pFileName) {
  int retVal = 0;
  CU_set_error_action(CUEA_IGNORE);
  /* setup tests */
  if (!(retVal = CU_initialize_registry())) {
    /* add tests cases */
    CU_register_suites(suites);
    /* setup output options */
    CU_set_output_filename(pFileName);
    CU_basic_set_mode(CU_BRM_VERBOSE);
    CU_list_tests_to_file();
    /* run tests */
    CU_automated_run_tests();
    CU_basic_run_tests();
    /* shutdown */
    CU_cleanup_registry();
  }
  return retVal;  
}
/* ------------ */
int
main(void) {
  return _runTests(OUTPUT_FILE);
}
