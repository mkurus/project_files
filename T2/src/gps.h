#ifndef GPS_H_
#define GPS_H_

#define GPS_BAUDRATE_10HZ           115200
#define GPS_INITIAL_BAUD_RATE       9600
#define GPS_BAUDRATE_DETECT_DELAY   300
#define GPS_MESSAGE_BUFFER_SIZE     128
#define GPS_UART_SRB_SIZE           128	                 /* Send ring buffer size */
#define GPS_UART_RRB_SIZE           (2048)	                 /* Receive ring buffer size */

#define NMEA_FRACTIONAL_SCALER               100000
#define GPS_POS_DEGREES_SCALER               100000
#define METERS_IN_KNOTS                      1852                     /* 1 knot = 1.852 kmh */
#define MAX_DISTANCE_BETWEEN_GPS_MESSAGES    (10.0)
#define IDLE_SPEED_THRESHOLD                       2
#define MIN_NUMBER_OF_SATS_FOR_MOVING_NOT_IGNITED  5
#define FLASH_KM_UPDATE_DISTANCE                   1000      /* update on every 1 km */

#define NMEA_MSG_DELIMITER   ','
#define G_VAL_WINDOW_SIZE     5


#define GPS_DATA_IDLE_TIMEOUT  2
typedef struct GPS_ACC_INFO{
	float g;
	float filtered_g;
	float speed;
}GPS_ACC_INFO_T;

typedef struct GPS_POSITION_DATA {
	/** Latitude  as a number of degrees scaled by  GPS_POS_DEGREES_SCALER */
	int32_t latitude;
	/** Longitude as a number of degrees scaled b  GPS_POS_DEGREES_SCALER */
	int32_t longitude;
	/** Altitude  as a number of absolute meters */
	uint16_t altitude;
	/* distance */
	double distance;
}GPS_POSITION_DATA_T;

/* Enum for mode 1 GPGSA */
typedef enum NMEA_GPGSA_MODE1 {
	/** Manual, forced to operate in 2D or 3D mode */
	NMEA_GSA_MODE1_MANUAL    = 'M',
	/** 2D automatic, allowed to automatically switch 2D/3D */
	NMEA_GSA_MODE1_AUTOMATIC = 'A',
}NMEA_GSA_MODE1_T;

/* Enum for mode 2 GPGSA */
typedef enum NMEA_GPGSA_MODE2 {
	/** Fix not available */
	NMEA_GSA_MODE2_NO_FIX = 1,
	/** 2D */
	NMEA_GSA_MODE2_2D     = 2,
	/** 3D */
	NMEA_GSA_MODE2_3D     = 3,
}NMEA_GSA_MODE2_T;

enum nmea_data_type {
	/** Unknown input */
	NMEA_TYPE_UNKNOWN,
	/** Recommended Minimum Specific GNSS Data */
	NMEA_TYPE_RMC,
	/** Time, position and fix type data */
	NMEA_TYPE_GGA,
	/** GPS receiver operating mode, active satellites used in the position
	 *  solution and DOP values */
	NMEA_TYPE_GSA,
};

typedef enum NMEA_RMC_STATUS{
	/* valid sentence */
	NMEA_RMC_VALID = 'A',
	/* invalid sentence */
	NMEA_GSA_INVALID = 'V'
}NMEA_RMC_STATUS_T;

/* Enum for the NMEA GPS indicators */
enum nmea_gps_coord_indicator {
	/** North */
	NMEA_GPS_COORD_NORTH = 'N',
	/** South */
	NMEA_GPS_COORD_SOUTH = 'S',
	/** West */
	NMEA_GPS_COORD_WEST  = 'W',
	/** East */
	NMEA_GPS_COORD_EAST  = 'E'
};

/* Structure for GPS latitude and longitude coordinates */
struct nmea_gps_coord_val {
	/** Indicator (N, S, E, W) */
	enum nmea_gps_coord_indicator indicator;
	/** Degrees */
	int16_t degrees;
	/** Minutes */
	int16_t minutes;
	/** Minutes (fractional) */
	int32_t minutes_frac;
};

/* Structure for GPS coordinates, latitude and longitude */
struct nmea_gps_coord {
	/** Latitude */
	struct nmea_gps_coord_val latitude;
	/** Longitude */
	struct nmea_gps_coord_val longitude;
};

typedef struct RMC_MESSAGE {
	/* UTC hour */
	uint8_t utc_hour;
	/* UTC minute */
	uint8_t utc_minute;
	/* UTC second */
	uint8_t utc_second;
	/* status */
	NMEA_RMC_STATUS_T status;
	/* GPS coordinates */
	struct nmea_gps_coord coords;
	/* Speed, knots (integer)  */
	uint16_t speed;
	/* Speed, knots (fractional) */
	uint16_t speed_frac;
	float f_speed;
	/* Course (integer) */
	uint16_t course;
	/* Course (fractional) */
	uint16_t course_frac;
	/* Date (day) */
	uint8_t day;
	/* Date (month) */
	uint8_t month;
	/* Date (year) */
	uint8_t year;
}RMC_MESSAGE_T;

typedef struct GGA_MESSAGE {
	/** UTC hour */
	uint8_t utc_hour;
	/** UTC minute */
	uint8_t utc_minute;
	/** UTC second */
	uint8_t utc_second;
	/** GPS coordinates */
	struct nmea_gps_coord coords;
	/** Position fix indicator */
	uint8_t position_fix;
	/** Number of satellites used */
	uint8_t satellites;
	/** Altitude */
	int16_t altitude;
}GGA_MESSAGE_T;

/* Structure for GPGSA data
 */
typedef struct GSA_MESSAGE {
	/** Mode 1 */
	NMEA_GSA_MODE1_T mode1;
	/** Mode 2 - 2D or 3D */
	NMEA_GSA_MODE2_T mode2;
	/** Satellites used on the channels */
	uint8_t satellite[12];
	/** Position dilution of precision (integer)   */
	uint8_t pdop_int;
	/** Position dilution of precision (fractional) */
	uint8_t pdop_frac;
	/** Horizontal dilution of precision (integer) */
	uint8_t hdop_int;
	/** Horizontal dilution of precision (fractional) */
	uint8_t hdop_frac;
	/** Vertical dilution of precision (integer) */
	uint8_t vdop_int;
	/** Vertical dilution of precision (fractional) */
	uint8_t vdop_frac;
}GSA_MESSAGE_T;

typedef struct NMEA_MSG {

	enum nmea_data_type type;
	struct NMEA_MSG_ITEMS_T{

		RMC_MESSAGE_T rmc;
		GGA_MESSAGE_T gga;
		GSA_MESSAGE_T gsa;
	}nmea_msg_items_t;

} NMEA_MSG_T;

typedef struct NMEA_STRINGS {
	char* string;
	bool (*func)(const char *, NMEA_MSG_T *);

}NMEA_STRINGS_T;

typedef struct VEHICLE_POS_INFO
{
	NMEA_RMC_STATUS_T gpsValid;
	uint16_t speed;
	uint16_t course;
    float xAccel;
    float yAccel;
    float zAccel;
}VEHICLE_POS_INFO_T;

typedef enum GPS_BAUD_RATES{
	GPS_BAUD_RATE_4800 = 4800,
	GPS_BAUD_RATE_9600 = 9600,
	GPS_BAUD_RATE_19200 = 19200,
	GPS_BAUD_RATE_115200 = 115200
}GPS_BAUD_RATES_T;

void task_gps();

NMEA_GSA_MODE2_T gps_get_mode2();
NMEA_RMC_STATUS_T gps_get_status();

void gps_set_led_status(bool status);
void gps_init_info();
void gps_set_nmea_msg_freq(uint16_t nmea_output_freq);
void gps_send_mtk_command(const char *command);
void Init_PositionInfo();
void gps_get_rmc_info(RMC_MESSAGE_T *);
void gps_get_gsa_info(GSA_MESSAGE_T *);
void gps_get_gga_info(GGA_MESSAGE_T *);
void gps_get_position_info(GPS_POSITION_DATA_T *const);
void GpsCommUART_IsrHandler(void);
bool isKmRecordLimitExceeded();
bool gps_is_moving();
bool gps_init(uint32_t baud_rate);
uint16_t gps_get_speed();
uint8_t gps_get_position_fix();
uint8_t gps_get_number_of_satellites();
float gps_find_max_g();
float gps_find_min_g();
float gpsGetSpeed();
uint16_t gpsGetCourse();
float gps_calc_g(float current_speed);
uint16_t gps_find_pos_acc_start();
bool gps_get_healt_status();


#endif /* GPS_H_ */
