
#include "LiteButton.h"
#include <Arduino.h>

LiteButton::LiteButton(uint8_t pin, uint16_t debounce_ms)
:  _pin(pin)
,  _delay(debounce_ms)
,  _state(HIGH)
,  _ignore_until(0)
,  _is_changed(false)
{
}

void LiteButton::begin()
{
	pinMode(_pin, INPUT_PULLUP);
}

bool LiteButton::read()
{
	if (_ignore_until > millis())
	{
        
	}
	else if (digitalRead(_pin) != _state)
	{
		_ignore_until = millis() + _delay;
		_state = !_state;
		_is_changed = true;
	}
	
	return _state;
}

bool LiteButton::isToggled()
{
	read();
	return isChanged();
}


bool LiteButton::isChanged()
{
	if (_is_changed)
	{
		_is_changed = false;
		return true;
	}
	return false;
}

bool LiteButton::isPressed()
{
	return (read() == PRESSED && isChanged());
}

bool LiteButton::isReleased()
{
	return (read() == RELEASED && isChanged());
}