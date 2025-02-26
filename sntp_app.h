/***************************************************************************/ /**
 * @file app.h
 * @brief Top level application functions
 *******************************************************************************
 * # License
 * <b>Copyright 2020 Silicon Laboratories Inc. www.silabs.com</b>
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

#ifndef SNTP_APP_H
#define SNTP_APP_H
#include "stdint.h"

/***************************************************************************/ /**
 * Initialize application.
 ******************************************************************************/
void sntp_app_init(const void *unused);
uint32_t sntp_get_time_to_calendar(char *get_time_str);

#endif // SNTP_APP_H
