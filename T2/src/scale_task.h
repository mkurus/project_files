/*
 * scale_task.h
 *
 *  Created on: 27 Şub 2018
 *      Author: mkurus
 */

#ifndef SCALE_TASK_H_
#define SCALE_TASK_H_

#define    SCALE_TASK_EVENT_QUE_SIZE            5
#define    SCALE_CALIB_MEASUREMENT_INTERVAL     50
#define    SCALE_CALIBRATION_SAMPLE_COUNT        11
#define    SCALE_REFERENCE_WEIGHT               (1000.0)
#define    SCALE_DISCONNECT_TIMEOUT             300  /* if disconeected 3 secs. post a disconnect event */
/* events processed by scale task */
typedef enum {
	SIGNAL_SCALE_ACTIVATED,
	SIGNAL_SCALE_CALIBRATE_M,
	SIGNAL_SCALE_START_CONVERSION_TICK,
	SIGNAL_SCALE_START_CALIBRATION,
	SIGNAL_SCALE_INACTIVATED,
	SIGNAL_SCALE_DISCONNECT_TIMEOUT
}SCALE_TASK_SIGNAL_T;

typedef enum SCALE_TASK_STATE{
	SCALE_CALIBRATED_STATE,
	SCALE_CALIBRATE_C_STATE,
	SCALE_CALIBRATE_M_STATE
}SCALE_TASK_STATE_T;

/* event structure */
typedef struct{
	SCALE_TASK_SIGNAL_T  sig;
}SCALE_TASK_EVENT_T;

typedef struct {
	uint8_t        u8_wrtPtr;
	uint8_t        u8_readPtr;
	uint8_t        u8_count;
	SCALE_TASK_EVENT_T    tEventBuffer[SCALE_TASK_EVENT_QUE_SIZE];
}ScaleTaskEventQueue;    // circular event queue


typedef struct {
	SCALE_TASK_STATE_T tTaskState;
	ScaleTaskEventQueue tEventQueue;
}ScaleTask;

typedef struct{
	int32_t scaleVal;
	bool isConnected;
}SCALE_INFO_T;

void PutEvent_ScaleTask(SCALE_TASK_EVENT_T *tEventContainer);
void scale_task_init();
void task_scale();
void get_scale_info(SCALE_INFO_T *scaleContainer);
#endif /* SCALE_TASK_H_ */
