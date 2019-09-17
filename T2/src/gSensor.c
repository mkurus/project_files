#include "chip.h"
#include "board.h"
#include "timer.h"
#include "event.h"
#include "messages.h"
#include "utils.h"
#include "settings.h"
#include "trace.h"
#include "gps.h"
#include "MMA8652.h"
#include "gSensor.h"
#include <math.h>
#include <string.h>
#include <stdlib.h>

//G_ALARM_INFO g_alarm_queue[G_ALARM_QUEUE_SIZE];

/************************************************************/
void Init_DriverProfileTask()
{
//	  memset(g_alarm_queue ,0, sizeof(g_alarm_queue));
}
/************************************************************/
G_VALUES_T MovingAverage(G_VALUES_T *g_value_samples, uint8_t lag)
{
	G_VALUES_T movAvgBuffer;
	float xSum = 0;
	float ySum = 0;
	float zSum = 0;
	char printBuf[128];
	uint8_t i;

	for( i= 0; i < lag; i++){
		xSum += g_value_samples[i].xAccel;
		ySum += g_value_samples[i].yAccel;
		zSum += g_value_samples[i].zAccel;
		sprintf(printBuf,"Sampled Values: %f,%f,%f\n",g_value_samples[i].xAccel,
													 g_value_samples[i].yAccel,
													g_value_samples[i].zAccel);
	//	PRINT_K(printBuf);
	}

	movAvgBuffer.xAccel = xSum / lag;
	movAvgBuffer.yAccel = ySum / lag;
	movAvgBuffer.zAccel = zSum / lag;

	sprintf(printBuf,"\nWindow Average Values: %f,%f,%f\n",movAvgBuffer.xAccel,
														 movAvgBuffer.yAccel,
														 movAvgBuffer.zAccel);
	//PRINT_K(printBuf);

	return movAvgBuffer;
}
/************************************************************/
void ShiftBuffer(G_VALUES_T *buffer, uint16_t bufferSize)
{
	uint8_t i;

	for( i= 0; i< bufferSize; i++)
		buffer[i] = buffer[i+1];
}
/************************************************************/
bool check_sudden_deceleration(GPS_ACC_INFO_T *acc_info, uint16_t speed)
{
	static G_ALARM_INFO g_alarm;
	T_MSG_EVENT_T t_msg_event_t;
	static ACC_STATE acc_state = G_OVER_THRESHOLD;
	static uint16_t g_alarm_100_msecs = 0;
//	int8_t  alarm_queue_index = 0;
	char printBuf[256];

			switch(acc_state)
			{
			   	 case G_OVER_THRESHOLD:
			   	  if(acc_info[G_VAL_WINDOW_SIZE- 1].filtered_g <= DECELERATION_ALARM_HIGH_THRESHOLD){
			   			g_alarm_100_msecs = 1;
			   		    g_alarm.initial_speed = acc_info[G_VAL_WINDOW_SIZE - 2].speed; /* keep initial speed and g values*/
			   		    g_alarm.max_g = gps_find_min_g();
			   			PRINT_K("\n*********Ani yavaslama basladi*******\n");
			   		    acc_state = G_BELOW_THRESHOLD;
			   	  }
			   	  else
			   		    g_alarm_100_msecs = 0;
			   	break;

			   	case G_BELOW_THRESHOLD:
			   	if(acc_info[G_VAL_WINDOW_SIZE- 1].filtered_g <= DECELERATION_ALARM_LOW_THRESHOLD){
			   			g_alarm_100_msecs++;
			   			if(gps_find_min_g() < g_alarm.max_g)   /* update maximum g value*/
			   				g_alarm.max_g = gps_find_min_g();
			   	}
			   	else{
			   	    g_alarm.final_speed = acc_info[G_VAL_WINDOW_SIZE- 2].speed;
			    	g_alarm_100_msecs = g_alarm_100_msecs + 2;
			    	g_alarm.duration = g_alarm_100_msecs *100;
			   		g_alarm.max_g = g_alarm.max_g / GRAV_CONSTANT;
			   		if(g_alarm.initial_speed > g_alarm.final_speed + 1){
			   			memset(&t_msg_event_t, 0, sizeof(t_msg_event_t));
			   			strcpy(t_msg_event_t.msg_type, EVENT_DECELERATION);
			   			sprintf(t_msg_event_t.param1,"%d,%d,%d",g_alarm.duration,
			   					   						        g_alarm.initial_speed,
			   											        g_alarm.final_speed);

			   		    put_t_msg_event(&t_msg_event_t);

			   			sprintf(printBuf,"\nDecelerated %d ms. Max g value %f,Initial Speed %d\n,Final Speed %d\n,",
			   																						g_alarm_100_msecs*100,
			   																						g_alarm.max_g,
																									g_alarm.initial_speed,
																									g_alarm.final_speed);
			   			 PRINT_K(printBuf);
			   		}
			   		g_alarm.max_g =0;
			   		g_alarm_100_msecs = 0;
			   		acc_state = G_OVER_THRESHOLD;
			   	}
			   	break;
			}

}
/************************************************************/
bool check_sudden_acceleration(GPS_ACC_INFO_T *acc_info, uint16_t speed)
{
	static G_ALARM_INFO g_alarm;
	T_MSG_EVENT_T t_msg_event_t;
	static ACC_STATE acc_state = G_BELOW_THRESHOLD;
	static uint16_t g_alarm_100_msecs = 0;
	//int8_t  alarm_queue_index = 0;
	char printBuf[256];
//    uint16_t acc_start;

    		switch(acc_state)
    		{
		     case G_BELOW_THRESHOLD:
		    	 if(acc_info[G_VAL_WINDOW_SIZE - 1].filtered_g >= ACCELERATION_ALARM_HIGH_THRESHOLD){
		    		 g_alarm_100_msecs = 1;
		    		 g_alarm.initial_speed = acc_info[G_VAL_WINDOW_SIZE - 2].speed; /* keep initial speed and g values*/
		    		 g_alarm.max_g = gps_find_max_g();
		    		 PRINT_K("\n*********Ani hizlanma basladi*******\n");
		    		 acc_state = G_OVER_THRESHOLD;
		    	 }
		   	  else
		   		  g_alarm_100_msecs = 0;
		   	  break;

		   	  case G_OVER_THRESHOLD:
		   		   if(acc_info[G_VAL_WINDOW_SIZE - 1].filtered_g >= ACCELERATION_ALARM_LOW_THRESHOLD){

		   			   g_alarm_100_msecs++;
		   			   if(gps_find_max_g() > g_alarm.max_g)   /* update maximum g value*/
		   				   g_alarm.max_g = gps_find_max_g();
		   		   }

		   		   else {
		   			   g_alarm.final_speed = acc_info[G_VAL_WINDOW_SIZE - 2].speed;
		   			   g_alarm_100_msecs = g_alarm_100_msecs + 2;
		   			   g_alarm.duration = g_alarm_100_msecs *100;
		   			   g_alarm.max_g = g_alarm.max_g / GRAV_CONSTANT;
		   			   /* generate alarm */
		   			   if(g_alarm.final_speed > g_alarm.initial_speed){
		   				   memset(&t_msg_event_t, 0, sizeof(t_msg_event_t));
		   				   strcpy(t_msg_event_t.msg_type, EVENT_ACCELERATION);
		   				   sprintf(t_msg_event_t.param1,"%d,%d,%d",g_alarm.duration,
		   						                                   g_alarm.initial_speed,
																   g_alarm.final_speed);

		   				   put_t_msg_event(&t_msg_event_t);
		   				   sprintf(printBuf,"\nAccelerated %d ms.Max g value %f, Initial Speed %d\n,Final Speed %d",
		   							                                                         (g_alarm_100_msecs)*100,
																						      g_alarm.max_g,
																							  g_alarm.initial_speed,
																							  g_alarm.final_speed);
		   				   PRINT_K(printBuf);
		   				}
		   				g_alarm.max_g = 0;
		   				acc_state = G_BELOW_THRESHOLD;
		   		   }
		   		   break;
		   	   }
}
/************************************************************/
bool check_rough_road(float g_value, uint16_t speed)
{
	static G_ALARM_INFO g_alarm;
	static ACC_STATE  z_acc_state = G_BELOW_THRESHOLD;
	static uint16_t g_alarm_100_msecs = 0;
	int8_t  alarm_queue_index = 0;
//	char printBuf[256];

		switch(z_acc_state)
		{
			case G_BELOW_THRESHOLD:
			if(sqrt(pow(g_value,2))  > ROUGH_ROAD_Z_VALUE_THRESHOLD){
				PRINT_G("\n********Bozuk yol esik uzerinde*******\n");
			   g_alarm_100_msecs++;
			   g_alarm.initial_speed = speed;   /* keep initial speed and g values*/
			   if(sqrt(pow(g_value,2)) > sqrt(pow(g_alarm.max_g,2)))
				   g_alarm.max_g = g_value;

			   if(g_alarm_100_msecs >= ROUGH_ROAD_HIGH_THRESHOLD_TICK_COUNT){
				   		PRINT_G("\n*********Bozuk yol alarmi basladi*******\n");
				   		z_acc_state = G_OVER_THRESHOLD;
			    }
			}
			else
				 g_alarm_100_msecs = 0;
			break;

			case G_OVER_THRESHOLD:
			if(sqrt(pow(g_value,2)) > ROUGH_ROAD_Z_VALUE_THRESHOLD){
				 g_alarm_100_msecs++;
				 if(sqrt(pow(g_value,2)) > sqrt(pow(g_alarm.max_g,2)))  /* update maximum g value*/
				    g_alarm.max_g = g_value;
			}
	        /* higher g value  than threshold detected */
			else{
				PRINT_G("\n********Bozuk yol esik altinda*******\n");
				  g_alarm.final_speed = speed;
				  g_alarm.duration = g_alarm_100_msecs *100;
				  if(g_alarm_100_msecs >= ROUGH_ROAD_LOW_THRESHOLD_TICK_COUNT){
				   	 alarm_queue_index = get_free_alarm_buffer_index();
				   	 if(alarm_queue_index > -1){
				   	//	g_alarm_queue[alarm_queue_index] = g_alarm;
				   	//	strcpy(g_alarm_queue[alarm_queue_index].alarm_str, "BZK");
				   	//	g_alarm_queue[alarm_queue_index].alarm_exist = TRUE;
				   /*		sprintf(printBuf,"\nIvme degeri %d ms %f uzerinde kaldi.Max g value %f\n,", g_alarm_100_msecs*100,
				   							  	  	  	  	  	  	  	  	  	  	  	  	  	  	ROUGH_ROAD_Z_VALUE_THRESHOLD,
																									g_alarm_queue[alarm_queue_index].max_g);
				   		PRINT_G(printBuf);*/
				   	}
				   	g_alarm.max_g = 0;
				   	g_alarm_100_msecs = 0;
				   	z_acc_state = G_BELOW_THRESHOLD;
		         }
			}
		 break;
	}

}
/************************************************************/
bool check_xy_acceleration(uint16_t speed, uint16_t heading, float x_axis_g, float y_axis_g)
{
	static uint16_t prev_heading;
	static G_ALARM_INFO g_alarm;
	static uint16_t g_alarm_100_msecs = 0;
	int8_t  alarm_queue_index;
	static ACC_STATE  xy_acc_state = G_BELOW_THRESHOLD;
//	char printBuf[256];

	float total_g = sqrt(pow(x_axis_g,2)+pow(y_axis_g,2));

	switch(xy_acc_state)
	{
		case G_BELOW_THRESHOLD:
		if((total_g  > XY_ACCELERATION_THRESHOLD_G) && abs(heading - prev_heading) < XY_HEADING_THRESHOLD_ANGLE){
			PRINT_G("\n********XY ivmelenme Esik uzerinde*******\n");
			g_alarm_100_msecs++;
			g_alarm.initial_speed = speed;   /* keep initial speed and g values*/
			if(total_g > g_alarm.max_g)
				g_alarm.max_g = total_g;

			if(g_alarm_100_msecs >= XY_ACCEL_THRESHOLD_TICK_COUNT){
				PRINT_G("\n*********XY ivmelenme alarmi basladi*******\n");
				xy_acc_state = G_OVER_THRESHOLD;
			}
		}
		else
			g_alarm_100_msecs = 0;
		break;

				case G_OVER_THRESHOLD:
				if(total_g > XY_ACCELERATION_THRESHOLD_G){
					 g_alarm_100_msecs++;
					 if(total_g > g_alarm.max_g)  /* update maximum g value*/
					    g_alarm.max_g = total_g;
				}
		        /* higher g value  than threshold detected */
				else{
					PRINT_G("\n******** XY ivmelenme alarmi Esik altinda*******\n");
					  g_alarm.final_speed = speed;
					  g_alarm.duration = g_alarm_100_msecs *100;
					  if(g_alarm_100_msecs >= XY_ACCEL_THRESHOLD_TICK_COUNT){
					   	 alarm_queue_index = get_free_alarm_buffer_index();
					   	 if(alarm_queue_index > -1){
					   	//	g_alarm_queue[alarm_queue_index] = g_alarm;
					   	//	strcpy(g_alarm_queue[alarm_queue_index].alarm_str, "XYA");
					   //		g_alarm_queue[alarm_queue_index].alarm_exist = TRUE;
					   	/*	sprintf(printBuf,"Ivme degeri %d ms %f uzerinde kaldi.Max g value %f\n,", g_alarm_100_msecs*100,
					   																				  XY_ACCELERATION_THRESHOLD_G,
					   																			      g_alarm_queue[alarm_queue_index].max_g);
					   		PRINT_G(printBuf);*/
					   	}
					   	g_alarm.max_g = 0;
					   	g_alarm_100_msecs = 0;
					   	xy_acc_state = G_BELOW_THRESHOLD;
			         }
				}
			 break;
		}
	prev_heading = heading;
}
/************************************************************/
bool check_xyz_acceleration(uint16_t speed, uint16_t heading, float x_axis_g, float y_axis_g,float z_axis_g)
{
	///static G_ALARM_INFO g_alarm;
	int8_t  alarm_queue_index ;
	char printBuf[256];

	float total_g = sqrt(pow(x_axis_g,2)+pow(y_axis_g,2)+pow(z_axis_g,2));

	if(total_g >= CRASH_DETECTION_THRESHOLD_G) {
			alarm_queue_index = get_free_alarm_buffer_index();
			if(alarm_queue_index > -1){
				//g_alarm_queue[alarm_queue_index] = g_alarm;
				//strcpy(g_alarm_queue[alarm_queue_index].alarm_str, "CRA");
			//	g_alarm_queue[alarm_queue_index].alarm_exist = TRUE;
				sprintf(printBuf,"XYZ Acceleretion:Ivme degeri %f uzerinde kaldi.Max g value %f\n,",CRASH_DETECTION_THRESHOLD_G,total_g);

				PRINT_G(printBuf);
			}
	}
}
/************************************************************/
void g_sensor_calculate_rotation(ROTATION *rotation, G_VALUES_T *mma8652_accel_values)
{
	rotation->pitch = atan(mma8652_accel_values->xAccel/sqrt(pow(mma8652_accel_values->yAccel,2)+pow(mma8652_accel_values->zAccel,2)))*180/3.14;
	rotation->roll  = atan(mma8652_accel_values->yAccel/sqrt(pow(mma8652_accel_values->xAccel,2)+pow(mma8652_accel_values->zAccel,2)))*180/3.14;
	rotation->theta = atan(sqrt(pow(mma8652_accel_values->xAccel,2)+pow(mma8652_accel_values->yAccel,2))+ mma8652_accel_values->zAccel)*180/3.14;
}
/************************************************************/
int8_t get_free_alarm_buffer_index()
{
	//uint8_t i;

	/*for(i =0; i< G_ALARM_QUEUE_SIZE; i++){
		if(!(g_alarm_queue[i].alarm_exist))
			return i;
	}*/
	return -1;
}
/************************************************************/
bool get_gsensor_alarm_status()
{
//	int i;
/*	for(i =0; i< G_ALARM_QUEUE_SIZE; i++){
		if(g_alarm_queue[i].alarm_exist){
			PRINT_K("\nSensor Alarm exist\n");
			return TRUE;
		}

	}*/
	return FALSE;

}
/************************************************************/
void get_next_gsensor_alarm(G_ALARM_INFO *g_alarm)
{
//	int i;

	/*for(i =0; i< G_ALARM_QUEUE_SIZE; i++){
		if(g_alarm_queue[i].alarm_exist){
			g_alarm_queue[i].alarm_exist = FALSE;
			*g_alarm = g_alarm_queue[i];
			 return;
		}

	}*/
}
