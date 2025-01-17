#ifndef __SERVO_H__
#define __SERVO_H__
void SetAngle(unsigned int duty);
void RegressMiddle(void);
void EngineTurnRight(void);
void EngineTurnLeft(void);
void S92RInit(void);
void Sg92RTask(void);
void stop_trans(void);
void half_stop_trans(void);
#endif