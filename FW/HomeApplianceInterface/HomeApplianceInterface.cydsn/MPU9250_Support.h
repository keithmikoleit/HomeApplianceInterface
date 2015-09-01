/* ========================================
 *
 * Copyright YOUR COMPANY, THE YEAR
 * All Rights Reserved
 * UNPUBLISHED, LICENSED SOFTWARE.
 *
 * CONFIDENTIAL AND PROPRIETARY INFORMATION
 * WHICH IS THE PROPERTY OF your company.
 *
 * ========================================
*/

#ifndef MPU9250_SUPPORT_H
#define MPU9250_SUPPORT_H

#include "main.h"
#include "inv_mpu.h"
#include "inv_mpu_dmp_motion_driver.h"
    
/* I2C communication */
unsigned long  MPU9250_I2C_Write(unsigned char Address, unsigned char RegisterAddr, unsigned char RegisterLen, unsigned char *RegisterValue);
unsigned long  MPU9250_I2C_Read(unsigned char Address, unsigned char RegisterAddr, unsigned char RegisterLen, unsigned char *RegisterValue);

/* System tick integration */
/* KLMZ TODO: Make this a real function */
int get_clock_ms(unsigned long *count);

#define BUF_SIZE        (256)
#define PACKET_LENGTH   (23)

#define PACKET_DEBUG    (1)
#define PACKET_QUAT     (2)
#define PACKET_DATA     (3)

struct rx_s {
    unsigned char header[3];
    unsigned char cmd;
};

/* Hal Structure for MPU state */
struct hal_s {
    unsigned char sensors;
    unsigned char dmpOn;
    unsigned char waitForTap;
    volatile unsigned char newGyro;
    unsigned short report;
    unsigned short dmpFeatures;
    unsigned char motionIntMode;
    struct rx_s rx;
};

/* UART communication to PC python script */

typedef enum {
    PACKET_TYPE_ACCEL = 0,
    PACKET_TYPE_GYRO,
    PACKET_TYPE_QUAT,
    PACKET_TYPE_TAP,
    PACKET_TYPE_ANDROID_ORIENT,
    PACKET_TYPE_PEDO,
    PACKET_TYPE_MISC
} eMPL_packet_e;

/**
 *  @brief      Send a quaternion packet via UART.
 *  The host is expected to use the data in this packet to graphically
 *  represent the device orientation. To send quaternion in the same manner
 *  as any other data packet, use eMPL_send_data.
 *  @param[in]  quat    Quaternion data.
 */
void eMPL_send_quat(long *quat);
/**
 *  @brief      Send a data packet via UART
 *  @param[in]  type    Contents of packet (PACKET_DATA_ACCEL, etc).
 *  @param[in]  data    Data (length dependent on contents).
 */
void eMPL_send_data(unsigned char packet_type, void *data);

#endif

/* [] END OF FILE */
