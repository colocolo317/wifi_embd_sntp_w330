/***************************************************************************/ /**
 * @file calendar_example.c
 * @brief Calendar example
 *******************************************************************************
 * # License
 * <b>Copyright 2023 Silicon Laboratories Inc. www.silabs.com</b>
 *******************************************************************************
 *
 * The licensor of this software is Silicon Laboratories Inc. Your use of this
 * software is governed by the terms of Silicon Labs Master Software License
 * Agreement (MSLA) available at
 * www.silabs.com/about-us/legal/master-software-license-agreement. This
 * software is distributed to you in Source Code format and is governed by the
 * sections of the MSLA applicable to Source Code.
 *
 ******************************************************************************/
#include "sl_si91x_calendar.h"
#include "rsi_debug.h"
#include "sl_si91x_clock_manager.h"
#include "calendar_app.h"

/*******************************************************************************
 ***************************  Defines / Macros  ********************************
 ******************************************************************************/
#define MAX_SECOND          60u        // Total seconds in one minute
#define MAX_MINUTE          60u        // Total minutes in one hour
#define MAX_HOUR            24u        // Total hours in one day
#define SECONDS_IN_HOUR     3600u      // Total seconds in one hour
#define UNIX_TEST_TIMESTAMP 1723186800u // Unix Time Stamp for 09/08/2024, 15:00:00
#define TAIPEI_TIME_ZONE_SHIFT 28800u
#define MS_DEBUG_DELAY      1000u      // Debug prints after every 1000 counts (callback trigger)

#define TEST_CENTURY      2u
#define TEST_YEAR         1u
#define TEST_MONTH        February
#define TEST_DAY_OF_WEEK  Friday
#define TEST_DAY          2u
#define TEST_HOUR         18u
#define TEST_MINUTE       10u
#define TEST_SECONDS      10u
#define TEST_MILLISECONDS 100u

#define ALARM_CENTURY      2u
#define ALARM_YEAR         1u
#define ALARM_MONTH        February
#define ALARM_DAY_OF_WEEK  Friday
#define ALARM_DAY          2u
#define ALARM_HOUR         18u
#define ALARM_MINUTE       10u
#define ALARM_SECONDS      15u
#define ALARM_MILLISECONDS 100u

#define CAL_RC_CLOCK 2u

#define SOC_PLL_CLK  ((uint32_t)(180000000)) // 180MHz default SoC PLL Clock as source to Processor
#define INTF_PLL_CLK ((uint32_t)(180000000)) // 180MHz default Interface PLL Clock as source to all peripherals
/*******************************************************************************
 **********************  Local Function prototypes   ***************************
 ******************************************************************************/
static void calendar_print_datetime(sl_calendar_datetime_config_t data);
#if defined(ALARM_EXAMPLE) && (ALARM_EXAMPLE == ENABLE)
static void on_alarm_callback(void);
boolean_t is_alarm_callback_triggered = false;
#endif
#if defined(SEC_INTR) && (SEC_INTR == ENABLE)
static void on_sec_callback(void);
boolean_t is_sec_callback_triggered = false;
#endif
#if defined(MILLI_SEC_INTR) && (MILLI_SEC_INTR == ENABLE)
static void on_msec_callback(void);
boolean_t is_msec_callback_triggered = false;
#endif
static void default_clock_configuration(void);
/*******************************************************************************
 **************************   GLOBAL FUNCTIONS   *******************************
 ******************************************************************************/
void unix_time_to_calendar(time_t unix, sl_calendar_datetime_config_t *date) {
  struct tm *time = gmtime(&unix);

  date->Century = ((time->tm_year / 100) % 4) + 1;
  date->Year    = (time->tm_year % 100);
  date->Month   = (time->tm_mon + 1);
  date->Day     = time->tm_mday;
  date->Hour    = time->tm_hour;
  date->Minute  = time->tm_min;
  date->Second  = time->tm_sec;
  date->MilliSeconds = 0;
  date->DayOfWeek = time->tm_wday;
}

// Function to configure clock on powerup
static void default_clock_configuration(void)
{
  // Core Clock runs at 180MHz SOC PLL Clock
  sl_si91x_clock_manager_m4_set_core_clk(M4_SOCPLLCLK, SOC_PLL_CLK);

  // All peripherals' source to be set to Interface PLL Clock
  // and it runs at 180MHz
  sl_si91x_clock_manager_set_pll_freq(INFT_PLL, INTF_PLL_CLK, PLL_REF_CLK_VAL_XTAL);
}
/*******************************************************************************
 * Calendar example initialization function
 ******************************************************************************/
void calendar_init(time_t sntp_get_time)
{
  sl_calendar_datetime_config_t datetime_config;
  sl_calendar_datetime_config_t get_datetime;
  sl_status_t status;

  // default clock configuration by application common for whole system
  default_clock_configuration();

  do
  {
    //Configuration of clock and initialization of calendar
    status = sl_si91x_calendar_set_configuration(CAL_RC_CLOCK);
    if (status != SL_STATUS_OK) {
      DEBUGOUT("sl_si91x_calendar_set_configuration: Invalid Parameters, Error Code : %lu \r\n", status);
      break;
    }
    DEBUGOUT("Successfully configured Calendar\r\n");
    sl_si91x_calendar_init();
#if 0
    //Setting datetime for Calendar
    status = sl_si91x_calendar_build_datetime_struct(&datetime_config,
                                                     TEST_CENTURY,
                                                     TEST_YEAR,
                                                     TEST_MONTH,
                                                     TEST_DAY_OF_WEEK,
                                                     TEST_DAY,
                                                     TEST_HOUR,
                                                     TEST_MINUTE,
                                                     TEST_SECONDS,
                                                     TEST_MILLISECONDS);
    if (status != SL_STATUS_OK) {
      DEBUGOUT("sl_si91x_calendar_build_datetime_struct: Invalid Parameters, Error Code : %lu \r\n", status);
      break;
    }
    DEBUGOUT("Successfully built datetime structure\r\n");
#endif
    unix_time_to_calendar( sntp_get_time + TAIPEI_TIME_ZONE_SHIFT, &datetime_config);
    status = sl_si91x_calendar_set_date_time(&datetime_config);
    if (status != SL_STATUS_OK) {
      DEBUGOUT("sl_si91x_calendar_set_date_time: Invalid Parameters, Error Code : %lu \r\n", status);
      break;
    }
    DEBUGOUT("Successfully set calendar datetime\r\n");
    // Printing datetime for Calendar
    status = sl_si91x_calendar_get_date_time(&get_datetime);
    if (status != SL_STATUS_OK) {
      DEBUGOUT("sl_si91x_calendar_get_date_time: Invalid Parameters, Error Code : %lu \r\n", status);
      break;
    }
    DEBUGOUT("Successfully fetched the calendar datetime \r\n");
    calendar_print_datetime(get_datetime);
    DEBUGOUT("\r\n");

#if defined(CLOCK_CALIBRATION) && (CLOCK_CALIBRATION == ENABLE)
    //Clock Calibration
    sl_si91x_calendar_calibration_init();
    clock_calibration_config_t clock_calibration_config;
    clock_calibration_config.rc_enable_calibration          = true;
    clock_calibration_config.rc_enable_periodic_calibration = true;
    clock_calibration_config.rc_trigger_time                = SL_RC_THIRTY_SEC;
    clock_calibration_config.ro_enable_calibration          = true;
    clock_calibration_config.ro_enable_periodic_calibration = true;
    clock_calibration_config.ro_trigger_time                = SL_RO_ONE_SEC;
    status = sl_si91x_calendar_rcclk_calibration(&clock_calibration_config);
    if (status != SL_STATUS_OK) {
      DEBUGOUT("sl_si91x_calendar_rcclk_calibration: Invalid Parameters, Error Code : %lu \r\n", status);
      break;
    }
    DEBUGOUT("Successfully performed clock calibration \r\n");
    sl_si91x_calendar_rtc_start();
#endif

#if defined(ALARM_EXAMPLE) && (ALARM_EXAMPLE == ENABLE)
    sl_calendar_datetime_config_t alarm_config;
    sl_calendar_datetime_config_t get_alarm;
    status = sl_si91x_calendar_build_datetime_struct(&alarm_config,
                                                     ALARM_CENTURY,
                                                     ALARM_YEAR,
                                                     ALARM_MONTH,
                                                     ALARM_DAY_OF_WEEK,
                                                     ALARM_DAY,
                                                     ALARM_HOUR,
                                                     ALARM_MINUTE,
                                                     ALARM_SECONDS,
                                                     ALARM_MILLISECONDS);
    if (status != SL_STATUS_OK) {
      DEBUGOUT("sl_si91x_calendar_build_datetime_struct: Invalid Parameters, Error Code : %lu \r\n", status);
      break;
    }
    DEBUGOUT("Successfully built datetime structure\r\n");
    //Setting the alarm configuration
    status = sl_si91x_calendar_set_alarm(&alarm_config);
    if (status != SL_STATUS_OK) {
      DEBUGOUT("sl_si91x_calendar_set_alarm: Invalid Parameters, Error Code : %lu \r\n", status);
      break;
    }
    DEBUGOUT("Successfully configured Alarm\r\n");
    status = sl_si91x_calendar_register_alarm_trigger_callback(on_alarm_callback);
    if (status != SL_STATUS_OK) {
      DEBUGOUT("sl_si91x_calendar_register_alarm_trigger_callback: Invalid Parameters, Error Code : %lu \r\n", status);
      break;
    }
    // Printing datetime for Alarm
    status = sl_si91x_calendar_get_alarm(&get_alarm);
    if (status != SL_STATUS_OK) {
      DEBUGOUT("sl_si91x_calendar_get_alarm: Invalid Parameters, Error Code : %lu \r\n", status);
      break;
    }
    DEBUGOUT("Successfully fetched the alarm datetime \r\n");
    calendar_print_datetime(get_alarm);
    DEBUGOUT("\r\n");
#endif

#if defined(SEC_INTR) && (SEC_INTR == ENABLE)
    //One second trigger
    status = sl_si91x_calendar_register_sec_trigger_callback(on_sec_callback);
    if (status != SL_STATUS_OK) {
      DEBUGOUT("sl_si91x_calendar_register_sec_trigger_callback: Invalid Parameters, Error Code : %lu \r\n", status);
      break;
    }
    DEBUGOUT("Successfully enabled one second trigger \r\n");
#endif

#if defined(MILLI_SEC_INTR) && (MILLI_SEC_INTR == ENABLE)
    //One millisecond trigger
    status = sl_si91x_calendar_register_msec_trigger_callback(on_msec_callback);
    if (status != SL_STATUS_OK) {
      DEBUGOUT("sl_si91x_calendar_register_msec_trigger_callback: Invalid Parameters, Error Code : %lu \r\n", status);
      break;
    }
    DEBUGOUT("Successfully enabled one milisecond trigger \r\n");
#endif

#if defined(TIME_CONVERSION) && (TIME_CONVERSION == ENABLE)
    uint32_t unix = UNIX_TEST_TIMESTAMP;
    uint32_t ntp  = 0;
    status        = sl_si91x_calendar_convert_unix_time_to_ntp_time(unix, &ntp);
    if (status != SL_STATUS_OK) {
      DEBUGOUT("sl_si91x_calendar_convert_unix_time_to_ntp_time: Invalid Parameters, Error Code : %lu \r\n", status);
      break;
    }
    DEBUGOUT("Unix Time: %lu\r\n", unix);
    DEBUGOUT("NTP Time: %lu\r\n", ntp);
    uint32_t unix_new = 0;
    status            = sl_si91x_calendar_convert_ntp_time_to_unix_time(ntp, &unix_new);
    if (status != SL_STATUS_OK) {
      DEBUGOUT("sl_si91x_calendar_convert_ntp_time_to_unix_time: Invalid Parameters, Error Code : %lu \r\n", status);
      break;
    }
    DEBUGOUT("NTP Time: %lu\r\n", ntp);
    DEBUGOUT("Unix Time: %lu\r\n", unix_new);
#endif
  } while (false);
}

/*******************************************************************************
 * Function will run continuously and will wait for trigger
 ******************************************************************************/
void calendar_process_action(void)
{
#if defined(ALARM_EXAMPLE) && (ALARM_EXAMPLE == ENABLE)
  if (is_alarm_callback_triggered) {
    DEBUGOUT("Alarm Callback is Triggered \r\n");
    is_alarm_callback_triggered = false;
  }
#endif
#if defined(SEC_INTR) && (SEC_INTR == ENABLE)
  if (is_sec_callback_triggered) {
    //DEBUGOUT("One Sec Callback is Triggered \r\n");
    is_sec_callback_triggered = false;
  }
#endif
#if defined(MILLI_SEC_INTR) && (MILLI_SEC_INTR == ENABLE)
  if (is_msec_callback_triggered) {
    DEBUGOUT("One Milli-Sec Callback triggered 1000 times\r\n");
    is_msec_callback_triggered = false;
  }
#endif
}

/*******************************************************************************
 * Function to print date and time from given structure
 * 
 * @param[in] data pointer to the datetime structure
 * @return none
 ******************************************************************************/
static void calendar_print_datetime(sl_calendar_datetime_config_t data)
{
  DEBUGOUT("\r\n***Calendar time****\r\n");
  DEBUGOUT("Time Format: hour:%d, min:%d, sec:%d, msec:%d\r\n", data.Hour, data.Minute, data.Second, data.MilliSeconds);
  DEBUGOUT("Date Format: DD/MM/YY: %.2d/%.2d/%.2d ", data.Day, data.Month, data.Year);
  DEBUGOUT(" Century: %d", data.Century);
}

/*******************************************************************************
 * Callback function of alarm, it is a periodic alarm
 * After the callback is triggered, new alarm is set according to the ALARM_TRIGGER_TIME
 * macro in header file.
 * 
 * @param none
 * @return none
 ******************************************************************************/
#if defined(ALARM_EXAMPLE) && (ALARM_EXAMPLE == ENABLE)
static void on_alarm_callback(void)
{
  is_alarm_callback_triggered = true;
}
#endif

/*******************************************************************************
 * Callback function of one second trigger
 * 
 * @param none
 * @return none
 ******************************************************************************/
#if defined(SEC_INTR) && (SEC_INTR == ENABLE)
static void on_sec_callback(void)
{
  static uint32_t sec = 0;
  DEBUGOUT("%5lu sec\r\n", sec++);
  is_sec_callback_triggered = true;
}
#endif

/*******************************************************************************
 * Callback function of one millisecond trigger
 * 
 * @param none
 * @return none
 ******************************************************************************/
#if defined(MILLI_SEC_INTR) && (MILLI_SEC_INTR == ENABLE)
static void on_msec_callback(void)
{
  static uint16_t temp = 0;
  temp++;
  if (temp >= MS_DEBUG_DELAY) {
    is_msec_callback_triggered = true;
    temp                       = 0;
  }
}
#endif
