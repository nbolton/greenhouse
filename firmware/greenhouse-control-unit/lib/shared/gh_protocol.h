#pragma once

#define GH_ADDR_MAIN 0x01
#define GH_ADDR_NODE_1 0x02
#define GH_ADDR_NODE_2 0x03

#define GH_CMD_ACK 0x01            // generic response
#define GH_CMD_ERROR 0x02          // something bad happened (d1: code)
#define GH_CMD_HELLO 0x03          // say hello (d1: sequence)
#define GH_CMD_TEMP_DEVS_REQ 0x10  // request temp device count
#define GH_CMD_TEMP_DEVS_RSP 0x11  // respond temp device count
#define GH_CMD_TEMP_DATA_REQ 0x12  // request temp data (d1: device index)
#define GH_CMD_TEMP_DATA_RSP 0x13  // respond temp data (d1 + d2: raw ow data)
#define GH_CMD_MOTOR_SPEED 0x20    // motor speed (d1: pwm duty)
#define GH_CMD_MOTOR_RUN 0x21      // motor run (d1: direction, d2: time)

#define GH_ERROR_BAD_CMD 0x01        // invalid command
#define GH_ERROR_BAD_SEQ 0x02        // duplicate sequence
#define GH_ERROR_BAD_MOTOR_CMD 0x10  // invalid motor command

#define GH_MOTOR_FORWARD 0x01
#define GH_MOTOR_REVERSE 0x02

// 6-byte datagram
// 0 = to address
// 1 = from address
// 2 = command
// 3 = sequence (detect duplicates)
// 4 = data 1
// 5 = data 2
#define GH_LENGTH 6
#define GH_TO(buf) buf[0]
#define GH_FROM(buf) buf[1]
#define GH_CMD(buf) buf[2]
#define GH_SEQ(buf) buf[3]
#define GH_DATA_1(buf) buf[4]
#define GH_DATA_2(buf) buf[5]
