/**
 ******************************************************************************
 * @file           : system_task.h
 * @brief          : 系统任务调度模块
 ******************************************************************************
 */

#ifndef __SYSTEM_TASK_H
#define __SYSTEM_TASK_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/* 函数声明 */
void SystemTask_Init(void);
void SystemTask_Run(void);

#ifdef __cplusplus
}
#endif

#endif /* __SYSTEM_TASK_H */
