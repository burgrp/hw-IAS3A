const int VALUE_UNKNOWN = -1;

struct AdcChannel {
	int pin;
	short* target;	
};

class IAS3: public i2c::hw::BufferedSlave, genericTimer::Timer {

	struct {
		short waterFlow = VALUE_UNKNOWN;
		short waterPressure = VALUE_UNKNOWN;
		short refrigerantPressure = VALUE_UNKNOWN;
	} data;

	AdcChannel adcChannels[2] = {
		{.pin = 1, .target = &data.waterPressure},
		{.pin = 2, .target = &data.refrigerantPressure}
	};

	int adcChannel;

public:

	void init(target::i2c::Peripheral* peripheral, int address) {
		BufferedSlave::init(peripheral, address, NULL, 0, (unsigned char*)&data, sizeof(data));
		for (int c = 0; c < sizeof(adcChannels) / sizeof(AdcChannel); c++) {
			target::GPIOA.MODER.setMODER(adcChannels[c].pin, 3);
		}
	}

	virtual void onTimer() {
		startAdcConversion();		
	}

	void startAdcConversion() {
		target::ADC.CHSELR.setCHSEL(1 << adcChannels[adcChannel].pin);
		target::ADC.CR.setADSTART(1);
	}

	void endOfConversion(int value) {
		*adcChannels[adcChannel].target = value;
		adcChannel++;
		if (adcChannel >= sizeof(adcChannels) / sizeof(AdcChannel)) {
			adcChannel = 0;
		}
		start(100);
	}

};

IAS3 ias3;

void interruptHandlerI2C1() {
	ias3.handleInterrupt();
}

void interruptHandlerADC() {	
	if (target::ADC.ISR.getEOC()) {
		ias3.endOfConversion(target::ADC.DR.getDATA());
	}
}

void initApplication() {

	target::RCC.AHBENR.setIOPAEN(true);
	
	// I2C peripheral
	target::GPIOA.AFRH.setAFRH(9, 4);
	target::GPIOA.AFRH.setAFRH(10, 4);
	target::GPIOA.MODER.setMODER(9, 2);
	target::GPIOA.MODER.setMODER(10, 2);
	target::RCC.APB1ENR.setC_EN(1, 1);
	target::NVIC.ISER.setSETENA(1 << target::interrupts::External::I2C1);

	// ADC peripheral
	target::RCC.APB2ENR.setADCEN(1);
	target::ADC.CR.setADEN(1);
	target::ADC.CFGR2 = 30 << 1; // CKMODE[1:0] = 01, broken svd definition
	target::ADC.SMPR.setSMPR(7);
	target::ADC.IER.setEOCIE(1);
	target::NVIC.ISER.setSETENA(1 << target::interrupts::External::ADC);
	
	ias3.init(&target::I2C1, 0x70);
	ias3.startAdcConversion();
}
