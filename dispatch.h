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
#ifndef DISPATCH_H
#define DISPATCH_H
/* ------------ */
typedef enum {
  DISPATCH_OK = 0,
  DISPATCH_THREAD_DEAD,
  DISPATCH_INVALID_OPERAND,
  DISPATCH_INVALID_OPERATION,
  DISPATCH_INTERNAL_ERR,
  DISPATCH_MEMORY_ERR,
  DISPATCH_MAXERR
} dispatch_error;
/* ------------ */
typedef struct dispatch_task * dispatch_task;
typedef void * (*dispatch_func)(dispatch_task pTask, void * pContext);
typedef void (*dispatch_cnlCb)(const dispatch_task pTask, void *pContext);
typedef void (*dispatch_endCb)(const dispatch_task pTask,
			       void * pContext,
			       void * pReturn);
/* ------------ */
dispatch_error dispatch_engine_init(void);
dispatch_error dispatch_engine_destroy(void);
dispatch_task dispatch_init(dispatch_func pFunc,
			    dispatch_cnlCb pCancel,
			    void * pContext);
dispatch_task dispatch_initEx(dispatch_func pFunc,
			      dispatch_cnlCb pCancel,
			      dispatch_endCb pEnd,
			      void * pContext);
dispatch_error dispatch_destroy(dispatch_task pTask);
dispatch_error dispatch_schedule(dispatch_task pTask);
dispatch_error dispatch_cancel(dispatch_task pTask);
dispatch_error dispatch_wait(dispatch_task pTask[], size_t length);
void * dispatch_return(dispatch_task pTask);
/* ------------ */
#endif /* DISPATCH_H */
