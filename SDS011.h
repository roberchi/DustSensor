// SDS011 dust sensor PM2.5 and PM10
// ---------------------------------
//
// By R. Zschiegner (rz@madavi.de)
// April 2016
//
// Documentation:
//		- The iNovaFitness SDS011 datasheet
//

#ifndef __SDS011_H
#define __SDS011_H

#if ARDUINO >= 100
	#include "Arduino.h"
#else
	#include "WProgram.h"
#endif

class SDS011 {
	public:
		SDS011(void);

		void begin(HardwareSerial* serial);

		int read(float *p25, float *p10);
		void sleep();
		void wakeup();
		void continuous_mode();
	private:
		uint8_t _pin_rx, _pin_tx;
		Stream *sds_data;
};

#endif