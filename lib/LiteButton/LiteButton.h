#ifndef Button_h
#define Button_h
#include "Arduino.h"

class LiteButton
{
    private:
    	bool isChanged();
		uint8_t  _pin;
		uint16_t _delay;
		bool     _state;
		uint32_t _ignore_until;
		bool     _is_changed;
	public:
		LiteButton(uint8_t pin, uint16_t debounce_ms = 100);
		void begin();
		bool read();
		bool isToggled();
		bool isPressed();
		bool isReleased();

		
		const static bool PRESSED = LOW;
		const static bool RELEASED = HIGH;
	
	
};

#endif