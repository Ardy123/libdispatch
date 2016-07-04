/*
 * Copyright (C) 2016 Ardavon Falls
 * This file is part of libdispatch
 *
 * Libdispatch is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as 
 * published by the Free Software Foundation, either version 3 of the 
 * License, or any later version.
 *
 * Libdispatch is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 */
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include "dispatch.h"
/* ------------ */
#define STACKSZ (64 * 1024)
#define THREAD_NAME "dispatch-%lu"
/* ------------ */
typedef enum {
  DISPATCH_INIT = 0,
  DISPATCH_SCHEDULED,
  DISPATCH_CANCELED,
  DISPATCH_RAN,
  DISPATCH_MAX
} task_state;
/* ------------ */
struct dispatch_task {
  dispatch_func m_run;
  dispatch_cnlCb m_cancel;
  dispatch_endCb m_end;
  pthread_mutex_t m_lock;
  pthread_mutex_t m_waitLock;
  pthread_cond_t m_wait;
  void * m_pCtx;
  void * m_pReturn;
  task_state m_state;
  dispatch_task m_pNxt;
};
/* ------------ */
typedef struct {
  pthread_mutex_t m_lock;
  pthread_cond_t m_signal;
  dispatch_task  m_pLst;
} dispatch_queue;
/* ------------ */
typedef struct {  
  size_t m_nThreads;
  int m_quit;
  dispatch_queue m_queue;
  pthread_t m_threads[0];
} dispatch_engine;
/* ------------ */
static dispatch_engine* gspTaskEngine;
/* ------------ */
static dispatch_error _initThreads(pthread_t[], const size_t);
static void _destroyThreads(pthread_t[], const size_t);
static void * _threadProc(void *);
static dispatch_error _initQueue(dispatch_queue *);
static dispatch_error _destroyQueue(dispatch_queue *);
static void _pushQueue(dispatch_queue *, dispatch_task);
static dispatch_task _popQueueNoLock(dispatch_queue *);
static dispatch_task _popQueue(dispatch_queue *, int *);
static int _isCallingFromThreads(pthread_t[], const size_t, const pthread_t);
/* ------------ */
dispatch_error
dispatch_engine_init(void) {  
  dispatch_error err = DISPATCH_OK;
  const unsigned nProcessors = (unsigned)sysconf(_SC_NPROCESSORS_ONLN);
  const size_t dispatchEngineSz =
    sizeof(dispatch_engine) + (nProcessors * sizeof(pthread_t));
  
  if (gspTaskEngine) {
    err = DISPATCH_INVALID_OPERATION;
    goto dispatch_engine_init_err;
  }
  if (!(gspTaskEngine = calloc(1, dispatchEngineSz))) {
    err = DISPATCH_MEMORY_ERR;
    goto dispatch_engine_init_err;
  }
  gspTaskEngine->m_nThreads = nProcessors;
  if (DISPATCH_OK != (err = _initQueue(&gspTaskEngine->m_queue))) {
    goto dispatch_engine_init_err1;
  }
  if (DISPATCH_OK !=
      (err = _initThreads(gspTaskEngine->m_threads, nProcessors))) {
    goto dispatch_engine_init_err2;
  } 
  return err;
 dispatch_engine_init_err2:
  _destroyQueue(&gspTaskEngine->m_queue);
 dispatch_engine_init_err1:
  free(gspTaskEngine), gspTaskEngine = NULL;
 dispatch_engine_init_err:
  return err;
}
/* ------------ */
dispatch_error
dispatch_engine_destroy(void) {
  if (gspTaskEngine) {
    /* signal exit */
    gspTaskEngine->m_quit = -1;
    pthread_mutex_lock(&gspTaskEngine->m_queue.m_lock);
    pthread_cond_broadcast(&gspTaskEngine->m_queue.m_signal);
    pthread_mutex_unlock(&gspTaskEngine->m_queue.m_lock);
    /* destroy all threads */
    _destroyThreads(gspTaskEngine->m_threads, gspTaskEngine->m_nThreads);
    /* destroy queue */
    _destroyQueue(&gspTaskEngine->m_queue);
    /* shutdown engine */
    free(gspTaskEngine);
    gspTaskEngine = NULL;
    return DISPATCH_OK;
  }
  return DISPATCH_INVALID_OPERATION;
}
/* ------------ */
dispatch_task
dispatch_init(dispatch_func pFunc, dispatch_cnlCb pCancel, void * pContext) {
  struct dispatch_task * pTask = calloc(1, sizeof(struct dispatch_task));
  if (!pTask || !pFunc || !pCancel) {
    goto dispatch_init_err;
  }
  pTask->m_run = pFunc;
  pTask->m_cancel = pCancel;
  pTask->m_pCtx = pContext;
  if (pthread_mutex_init(&pTask->m_lock, NULL)) {
    goto dispatch_init_err;
  }
  if (pthread_mutex_init(&pTask->m_waitLock, NULL)) {
    pthread_mutex_destroy(&pTask->m_lock);
    goto dispatch_init_err;
  }
  if (pthread_cond_init(&pTask->m_wait, NULL)) {
    pthread_mutex_destroy(&pTask->m_waitLock);
    pthread_mutex_destroy(&pTask->m_lock);
    goto dispatch_init_err;
  }
  return pTask;
 dispatch_init_err:
  if (pTask) free(pTask);
  return NULL;
}
/* ------------ */
dispatch_task
dispatch_initEx(dispatch_func pFunc,
		dispatch_cnlCb pCancel,
		dispatch_endCb pEnd,
		void * pContext) {
  struct dispatch_task * pTask = calloc(1, sizeof(struct dispatch_task));
  if (!pTask || !pFunc || !pCancel || !pEnd) {
    goto dispatch_initEx_err;
  }
  pTask->m_run = pFunc;
  pTask->m_cancel = pCancel;
  pTask->m_end = pEnd;
  pTask->m_pCtx = pContext;
  if (pthread_mutex_init(&pTask->m_lock, NULL)) {
    goto dispatch_initEx_err;
  }
  if (pthread_mutex_init(&pTask->m_waitLock, NULL)) {
    pthread_mutex_destroy(&pTask->m_lock);
    goto dispatch_initEx_err;
  }
  if (pthread_cond_init(&pTask->m_wait, NULL)) {
    pthread_mutex_destroy(&pTask->m_waitLock);
    pthread_mutex_destroy(&pTask->m_lock);
    goto dispatch_initEx_err;
  }
  return pTask;
 dispatch_initEx_err:
  if (pTask) free(pTask);
  return NULL; 
}
/* ------------ */
dispatch_error
dispatch_schedule(dispatch_task pTask) {
  dispatch_error retVal = DISPATCH_INVALID_OPERAND;
  if (pTask) {
    if (!_isCallingFromThreads(gspTaskEngine->m_threads,
			       gspTaskEngine->m_nThreads,
			       pthread_self())) {    
      switch (pTask->m_state) {
      case DISPATCH_INIT:
      case DISPATCH_CANCELED:
      case DISPATCH_RAN:
	pTask->m_state = DISPATCH_SCHEDULED;
	_pushQueue(&gspTaskEngine->m_queue, pTask);
	retVal = DISPATCH_OK;
	break;
      default:
	retVal = DISPATCH_INVALID_OPERATION;
	break;
      }
    } else {
      retVal = DISPATCH_INVALID_OPERATION;
    }
  }
  return retVal;
}
/* ------------ */
dispatch_error
dispatch_cancel(dispatch_task pTask) {
  dispatch_error retVal = DISPATCH_INVALID_OPERAND;
  if (pTask) {
    pthread_mutex_lock(&pTask->m_lock);
    switch (pTask->m_state) {
    case DISPATCH_SCHEDULED: {
      pTask->m_state = DISPATCH_CANCELED;
      retVal = DISPATCH_OK;
      break;
    }
    default:
      retVal = DISPATCH_INVALID_OPERATION;
    }
    pthread_mutex_unlock(&pTask->m_lock);    
  }
  return retVal;
}
/* ------------ */
dispatch_error
dispatch_wait(dispatch_task tasks[], size_t length) {
  /* verify arguments */
  if (NULL == tasks || 0 == length) {
    return DISPATCH_INVALID_OPERAND;
  }
  /* check that its not being called from inside a task */
  if (_isCallingFromThreads(gspTaskEngine->m_threads,
			    gspTaskEngine->m_nThreads,
			    pthread_self())) {
    return DISPATCH_INVALID_OPERATION;
  }    
  /* wait for jobs to finish */
  if (length > 0) {
    const unsigned index = length - 1;
    pthread_mutex_lock(&tasks[index]->m_waitLock);      
    while (DISPATCH_SCHEDULED == tasks[index]->m_state) {
      pthread_cond_wait(&tasks[index]->m_wait, &tasks[index]->m_waitLock);
    }
    pthread_mutex_unlock(&tasks[index]->m_waitLock);
    dispatch_wait(tasks, index);
  }
  return DISPATCH_OK;
}
/* ------------ */
void *
dispatch_return(dispatch_task pTask) {
  void * pRetVal = NULL;
  if (pTask) {
    pthread_mutex_lock(&pTask->m_lock);
    switch (pTask->m_state) {
    case DISPATCH_RAN: {
      pRetVal = pTask->m_pReturn;
      break;
    }
    default:
      break;
    }
    pthread_mutex_unlock(&pTask->m_lock);      
  }
  return pRetVal;  
}
/* ------------ */
dispatch_error
dispatch_destroy(dispatch_task pTask) {
  dispatch_error retVal = DISPATCH_INVALID_OPERAND;
  if (pTask) {
    pthread_mutex_lock(&pTask->m_lock);
    switch (pTask->m_state) {
    case DISPATCH_INIT:
    case DISPATCH_CANCELED:
    case DISPATCH_RAN:
      pthread_mutex_unlock(&pTask->m_lock);
      pthread_cond_destroy(&pTask->m_wait);
      pthread_mutex_destroy(&pTask->m_waitLock);
      pthread_mutex_destroy(&pTask->m_lock);      
      free(pTask);
      retVal = DISPATCH_OK;
      break;
    default:
      pthread_mutex_unlock(&pTask->m_lock);    	    
      retVal = DISPATCH_INVALID_OPERATION;
    }
  }
  return retVal;
}
/* ------------ */
static dispatch_error
_initThreads(pthread_t threads[], const size_t nThreads) {
  if (nThreads > 0) {    
    char threadName[32];
    cpu_set_t cpuset;
    pthread_attr_t attributes;
    const size_t index = nThreads - 1;
    CPU_ZERO(&cpuset);
    CPU_SET(index, &cpuset);
    snprintf(threadName, sizeof(threadName) - 1, THREAD_NAME, index);    
    if (pthread_attr_init(&attributes)) {
      return DISPATCH_INTERNAL_ERR;
    }
    if (pthread_attr_setstacksize(&attributes, STACKSZ)) {
      pthread_attr_destroy(&attributes);
      return DISPATCH_INTERNAL_ERR;
    }
    if (pthread_create(&threads[index],
		       &attributes,
		       _threadProc,
		       gspTaskEngine)) {
      pthread_attr_destroy(&attributes);
      return DISPATCH_INTERNAL_ERR;
    }
    /* if name isn't set, not a deal killing issue */
    pthread_setname_np(threads[index], threadName);
    /* if affinity isn't set, not a deal killing issue */
    pthread_setaffinity_np(threads[index], sizeof(cpu_set_t), &cpuset);
    pthread_attr_destroy(&attributes);
    return _initThreads(threads, index);
  }
  return DISPATCH_OK;
}
/* ------------ */
static void
_destroyThreads(pthread_t threads[], const size_t nThreads) {
  if (nThreads > 0) {
    const size_t index = nThreads - 1;
    pthread_join(threads[index], NULL);
    _destroyThreads(threads, index);
  }
}
/* ------------ */
static void * _threadProc(void * pUser) {
  dispatch_engine * pEngine = (dispatch_engine *)pUser;
  while(0 == pEngine->m_quit) {    
    dispatch_task pTask = _popQueue(&pEngine->m_queue, &pEngine->m_quit);
    if (pTask) {
      /* make sure task is in proper state (not canceled, etc..) */    
      if (DISPATCH_SCHEDULED == pTask->m_state) {
	/* execute task */
	pTask->m_pReturn = pTask->m_run(pTask, pTask->m_pCtx);
	/* update task state */
	pthread_mutex_lock(&pTask->m_lock);
	if (DISPATCH_SCHEDULED == pTask->m_state) {
	  pTask->m_state = DISPATCH_RAN;
	}
	pthread_mutex_unlock(&pTask->m_lock);
      }
      /* invoke finished or canceled callback */
      if (pTask->m_end && DISPATCH_RAN == pTask->m_state) {
	pTask->m_end(pTask, pTask->m_pCtx, pTask->m_pReturn);
      } else if (pTask->m_cancel && DISPATCH_CANCELED == pTask->m_state) {
	pTask->m_cancel(pTask, pTask->m_pCtx);
      }
      /* unlock condition variable for wait */     
      pthread_mutex_lock(&pTask->m_waitLock);
      pthread_cond_signal(&pTask->m_wait);
      pthread_mutex_unlock(&pTask->m_waitLock);
    }
  }
  return pUser;
}
/* ------------ */
static dispatch_error
_initQueue(dispatch_queue * pQueue) {
  dispatch_error err = DISPATCH_OK;  
  if (pthread_mutex_init(&pQueue->m_lock, NULL)) {
    err = DISPATCH_INTERNAL_ERR; 
  } else if (pthread_cond_init(&pQueue->m_signal, NULL)) {
    pthread_mutex_destroy(&pQueue->m_lock);
    err = DISPATCH_INTERNAL_ERR; 
  }
  return err;
}
/* ------------ */
static dispatch_error
_destroyQueue(dispatch_queue * pQueue) {
  dispatch_task pTask= NULL;
  dispatch_error err = DISPATCH_OK;
  /* cancel all tasks on Queue */
  pthread_mutex_lock(&pQueue->m_lock);
  while (NULL != (pTask = _popQueueNoLock(pQueue))) {
    pTask->m_state = DISPATCH_CANCELED;
    if (pTask->m_cancel) {
      pTask->m_cancel(pTask, pTask->m_pCtx);
    }
  }
  pthread_mutex_unlock(&pQueue->m_lock);
  /* destroy lock & signal */
  if (pthread_cond_destroy(&pQueue->m_signal) ||
      pthread_mutex_destroy(&pQueue->m_lock)) {
    err = DISPATCH_INTERNAL_ERR;     
  }
  return err;
}
/* ------------ */
static void
_pushQueue(dispatch_queue * pQueue, dispatch_task pTask) {
  dispatch_task * ppNode;
  pthread_mutex_lock(&pQueue->m_lock);
  for (ppNode = &pQueue->m_pLst; *ppNode; ppNode = &((*ppNode)->m_pNxt)) {}
  *ppNode = pTask;
  pthread_cond_signal(&pQueue->m_signal);
  pthread_mutex_unlock(&pQueue->m_lock);
}
/* ------------ */
static dispatch_task
_popQueueNoLock(dispatch_queue * pQueue) {
  dispatch_task pTask = pQueue->m_pLst;
  pQueue->m_pLst = (pTask) ? pTask->m_pNxt : NULL;
  return pTask;  
}
/* ------------ */
static dispatch_task
_popQueue(dispatch_queue * pQueue, int * pExit) {
  dispatch_task pTask;
  pthread_mutex_lock(&pQueue->m_lock);
  while (!pQueue->m_pLst && !*pExit) {
    pthread_cond_wait(&pQueue->m_signal, &pQueue->m_lock);
  }
  pTask = _popQueueNoLock(pQueue);
  pthread_mutex_unlock(&pQueue->m_lock);
  return pTask;
}
/* ------------ */
static int
_isCallingFromThreads(pthread_t threads[],
		      const size_t nThreads,
		      const pthread_t threadId) {
  if (nThreads > 0) {
    const size_t index = nThreads - 1;    
    return ((threadId == threads[index]) |
	    _isCallingFromThreads(threads, index, threadId));
  }
  return 0;
}
