#include "Door.h"

Door::Door(const char * name, const byte sensor_pin, const byte motor_a_pin, const byte motor_b_pin, const byte motor_pwm_pin)
	:_name(name), _sensor_pin(sensor_pin), _motor_a_pin(motor_a_pin), _motor_b_pin(motor_b_pin), _motor_pwm_pin(motor_pwm_pin), status(Uninitialized) {}

void Door::begin()
{	 
	if (status != Uninitialized)
		return;

	pinMode(_sensor_pin, INPUT_PULLUP);	// configure door sensor pin as input with pull-up enabled

	pinMode(_motor_a_pin, OUTPUT);		// configure motor pins a output
	pinMode(_motor_b_pin, OUTPUT);
	pinMode(_motor_pwm_pin, OUTPUT);

	digitalWrite(_motor_a_pin, LOW);	// turn motor off
	digitalWrite(_motor_b_pin, LOW);
	digitalWrite(_motor_pwm_pin, LOW);

	// lock & unlock methods require status to be different from Uninitialized to run so we set dummy status
	status = Open;

	if (digitalRead(_sensor_pin) == DOOR_SENSOR_CLOSED)
	{	

		lock();
		_was_opened = false;
	}
	else
	{
		unlock();
		// unlock sets status to Closed internally, but we know door is open so we reset it
		status = Open;
		_was_opened = true;
	}

	_last_sensor_status = digitalRead(_sensor_pin);
	_sensor_millis = millis();

}

byte Door::check()
{
	// TODO - handle timer overflows
	// unlock, wait 60s, if dor closed lock
// unlock, door open, door close, wait 1s and lock
// unlock, door open, wait 60s and sound if not closed
	DoorStatus debounced_status = status;

	unsigned long elapsed_millis = millis() - _unlock_millis;
	bool sensor_status = digitalRead(_sensor_pin);
	if (_last_sensor_status != sensor_status)
	{
		_sensor_millis = millis();
		_last_sensor_status = sensor_status;
	}
	else if (_last_sensor_status == sensor_status && millis() - _sensor_millis > SENSOR_DEBOUNCE)
	{
		if (sensor_status == DOOR_SENSOR_OPEN)
			debounced_status = Open;
		else
			debounced_status = Closed;
	}

	switch(status)
	{
		case Uninitialized:
			 return CHECK_ERROR;
		case Locked:
			if (debounced_status == Open)
			{	
				Serial.print("Alarm");
				return CHECK_ALARM;
			}
			break;
		case Open:
			if (debounced_status == Closed && elapsed_millis > LOCK_TIMEOUT)
			//if (sensor_status == DOOR_SENSOR_CLOSED && _was_opened && elapsed_millis > LOCK_TIMEOUT)
				lock();
			//else if (!_was_opened && elapsed_millis > GRANT_ACCESS_TIMEOUT)
//				lock();
			else if (debounced_status == Open && elapsed_millis > CLOSE_ALERT_TIMEOUT)
			{
				Serial.print("Alert");
				return CHECK_ALERT;
			}
			break;
		case Closed:
			if (debounced_status == Closed && elapsed_millis > GRANT_ACCESS_TIMEOUT)
				lock();
			// TODO - wrong start time
			else if (debounced_status == Open)
				status = Open;
			break;
	}

	return CHECK_OK;
}

void Door::print()
{
	Serial.print(_name);
	Serial.print(" door status '");

	switch (status)
	{
		case Open: Serial.print("Open"); break;
		case Closed: Serial.print("Closed"); break;
		case Locked: Serial.print("Locked"); break;
		case Uninitialized: Serial.print("Uninitialized"); break;
	}

	Serial.print("', sensor reads ");
	if (digitalRead(_sensor_pin) == DOOR_SENSOR_CLOSED)
		Serial.print("'Closed'");
	else
		Serial.print("'Open'");

	Serial.print(", ");
	Serial.print(millis() - _unlock_millis);
	Serial.println("ms elapsed");

}

void Door::unlock()
{
	if (status == Uninitialized)
		return;

	cycleMotor(_motor_b_pin);
	_unlock_millis = millis();
	status = Closed;
	_was_opened = false;
}

void Door::lock()
{
	if (status == Uninitialized)
		return;

	cycleMotor(_motor_a_pin);
	status = Locked;
}

void Door::cycleMotor(byte high_pin)
{
	
	digitalWrite(LED_BUILTIN, HIGH);

	digitalWrite(high_pin, HIGH);

	for (int i = 0; i < 255 ; i++)
	{
		analogWrite(_motor_pwm_pin, i);
		delayMicroseconds(500);
	}

	digitalWrite(_motor_pwm_pin, HIGH);  
  
	for (int i = 255; i >= 0 ; i--)
	{
		analogWrite(_motor_pwm_pin, i);
		delay(2);
	}

	// Motor off
	digitalWrite(high_pin, LOW);
	digitalWrite(_motor_pwm_pin, LOW);

	digitalWrite(LED_BUILTIN, LOW);
}
