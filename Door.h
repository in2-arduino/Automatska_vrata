#ifndef Door_h_
#define Door_h_

#if ARDUINO >= 100
	#include "Arduino.h"
#else
	#include "WProgram.h"
#endif

#define DOOR_SENSOR_CLOSED LOW		// map sensor states to door states
#define DOOR_SENSOR_OPEN HIGH

#define SENSOR_DEBOUNCE 100			// timeouts in milliseconds
#define LOCK_TIMEOUT 500
#define GRANT_ACCESS_TIMEOUT 60000
#define CLOSE_ALERT_TIMEOUT 120000

#define CHECK_OK 0					// check method return values
#define CHECK_ALERT 1
#define CHECK_ALARM 2
#define CHECK_ERROR 3

class Door
{
	public:
		enum DoorStatus {Open, Closed, Locked, Uninitialized};

	private:

		const char * _name;
		const byte _sensor_pin;
		const byte _motor_a_pin;
		const byte _motor_b_pin;
		const byte _motor_pwm_pin;
		
		unsigned long _unlock_millis;
		unsigned long _sensor_millis;

		bool _last_sensor_status;
		bool _was_opened;

		void cycleMotor(byte high_pin);

	public:
		DoorStatus status;
		
		Door(const char * name, const byte sensor_pin, const byte motor_a_pin, const byte motor_b_pin, const byte motor_pwm_pin);
		
		void begin();
		void unlock();
		void lock();
		byte check();
		void print();
};
#endif