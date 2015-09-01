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
#include <project.h>
#include "MPU9250_Support.h"
#include "stdio.h"

static uint8 status;

unsigned long  MPU9250_I2C_Write(unsigned char Address, unsigned char RegisterAddr, unsigned char RegisterLen, unsigned char *RegisterValue)
{
    uint8 i = RegisterLen;
    
    /* Send Start with Address + Write */
    status = I2C_I2CMasterSendStart((uint32)Address, I2C_I2C_WRITE_XFER_MODE);
    /* Send register address */
    if(status == I2C_I2C_MSTR_NO_ERROR)
    {
        status |= I2C_I2CMasterWriteByte((uint32)RegisterAddr);
    }
    if(status == I2C_I2C_MSTR_NO_ERROR)
    {
        /* Send data one byte at a time */
        while(i-- && (status == I2C_I2C_MSTR_NO_ERROR))
        {
            status |= I2C_I2CMasterWriteByte((uint32)*RegisterValue++);
            // Delay to allow DMP time to setle
            CyDelayUs(10);
        }
    }
    /* Send Stop */
    status |= I2C_I2CMasterSendStop();
    
    return status;
}

unsigned long  MPU9250_I2C_Read(unsigned char Address, unsigned char RegisterAddr, unsigned char RegisterLen, unsigned char *RegisterValue)
{
    uint8 i = 0;
    
    /* Send Start with Address + Write */
    status = I2C_I2CMasterSendStart((uint32)Address, I2C_I2C_WRITE_XFER_MODE);
    if(status == I2C_I2C_MSTR_NO_ERROR)
    {
        /* Send register address */
        status |= I2C_I2CMasterWriteByte((uint32)RegisterAddr);
    }
    if(status == I2C_I2C_MSTR_NO_ERROR)
    {
        /* Send Start with Address + Read */
        status |= I2C_I2CMasterSendRestart((uint32)Address, I2C_I2C_READ_XFER_MODE);
    }
    if(status == I2C_I2C_MSTR_NO_ERROR)
    {
        /* Read in data bytes */
        while(i < (RegisterLen - 1))
        {
            RegisterValue[i] = I2C_I2CMasterReadByte(I2C_I2C_ACK_DATA);
            // Delay to allow DMP time to setle
            CyDelayUs(10);
            i++;
        }
        /* Read and NAK the final byte */
        RegisterValue[i] = I2C_I2CMasterReadByte(I2C_I2C_NAK_DATA);
    }

    /* Send Stop to indicate end of communciation */
    status |= I2C_I2CMasterSendStop();
    
    return status;
}

int get_clock_ms(unsigned long *count)
{
    return *count;
}

void eMPL_send_quat(long *quat)
{
    uint8 out[PACKET_LENGTH];
    if (!quat)
        return;
    memset(out, 0, PACKET_LENGTH);
    out[0] = '$';
    out[1] = PACKET_QUAT;
    out[3] = (char)(quat[0] >> 24);
    out[4] = (char)(quat[0] >> 16);
    out[5] = (char)(quat[0] >> 8);
    out[6] = (char)quat[0];
    out[7] = (char)(quat[1] >> 24);
    out[8] = (char)(quat[1] >> 16);
    out[9] = (char)(quat[1] >> 8);
    out[10] = (char)quat[1];
    out[11] = (char)(quat[2] >> 24);
    out[12] = (char)(quat[2] >> 16);
    out[13] = (char)(quat[2] >> 8);
    out[14] = (char)quat[2];
    out[15] = (char)(quat[3] >> 24);
    out[16] = (char)(quat[3] >> 16);
    out[17] = (char)(quat[3] >> 8);
    out[18] = (char)quat[3];
    out[21] = '\r';
    out[22] = '\n';
    
    DUART_SpiUartPutArray(out, PACKET_LENGTH);
}

void eMPL_send_data(unsigned char packet_type, void *data)
{
    uint8 buf[PACKET_LENGTH], length;

    memset(buf, 0, PACKET_LENGTH);
    buf[0] = '$';
    buf[1] = packet_type;
    buf[21] = '\r';
    buf[22] = '\n';

    if (packet_type == PACKET_TYPE_ACCEL || packet_type == PACKET_TYPE_GYRO) {
        short *sdata = (short*)data;
        buf[2] = (char)(sdata[0] >> 8);
        buf[3] = (char)sdata[0];
        buf[4] = (char)(sdata[1] >> 8);
        buf[5] = (char)sdata[1];
        buf[6] = (char)(sdata[2] >> 8);
        buf[7] = (char)sdata[2];
        length = 8;
        length = sprintf((char*)buf, "X: %hd Y: %hd Z: %hd\r\n", sdata[0], sdata[1], sdata[2]);
    } else if (packet_type == PACKET_TYPE_QUAT) {
        long *ldata = (long*)data;
        buf[2] = (char)(ldata[0] >> 24);
        buf[3] = (char)(ldata[0] >> 16);
        buf[4] = (char)(ldata[0] >> 8);
        buf[5] = (char)ldata[0];
        buf[6] = (char)(ldata[1] >> 24);
        buf[7] = (char)(ldata[1] >> 16);
        buf[8] = (char)(ldata[1] >> 8);
        buf[9] = (char)ldata[1];
        buf[10] = (char)(ldata[2] >> 24);
        buf[11] = (char)(ldata[2] >> 16);
        buf[12] = (char)(ldata[2] >> 8);
        buf[13] = (char)ldata[2];
        buf[14] = (char)(ldata[3] >> 24);
        buf[15] = (char)(ldata[3] >> 16);
        buf[16] = (char)(ldata[3] >> 8);
        buf[17] = (char)ldata[3];
        length = 18;
    } else if (packet_type == PACKET_TYPE_TAP) {
        buf[2] = ((char*)data)[0];
        buf[3] = ((char*)data)[1];
        length = 4;
    } else if (packet_type == PACKET_TYPE_ANDROID_ORIENT) {
        buf[2] = ((char*)data)[0];
        length = 3;
    } else if (packet_type == PACKET_TYPE_PEDO) {
        long *ldata = (long*)data;
        buf[2] = (char)(ldata[0] >> 24);
        buf[3] = (char)(ldata[0] >> 16);
        buf[4] = (char)(ldata[0] >> 8);
        buf[5] = (char)ldata[0];
        buf[6] = (char)(ldata[1] >> 24);
        buf[7] = (char)(ldata[1] >> 16);
        buf[8] = (char)(ldata[1] >> 8);
        buf[9] = (char)ldata[1];
        length = 10;
        // KLMZ Debug for terminal testing
        length = sprintf((char*)buf, "Steps: %ld Walk Time: %ld\r\n", ldata[0], ldata[1]);
    } else if (packet_type == PACKET_TYPE_MISC) {
        buf[2] = ((char*)data)[0];
        buf[3] = ((char*)data)[1];
        buf[4] = ((char*)data)[2];
        buf[5] = ((char*)data)[3];
        length = 6;
    }

    DUART_SpiUartPutArray(buf, length);
}


/* [] END OF FILE */
