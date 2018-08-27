const int VALUE_UNKNOWN = -1;

const int ADC_CHANNEL_WATER_PRESSURE = 1;
const int ADC_CHANNEL_REFRIGERANT_PRESSURE = 2;

class IAS3: public i2c::hw::BufferedSlave, genericTimer::Timer {

	int conv = 0;

	struct {
		short waterFlow = VALUE_UNKNOWN;
		short waterPressure = VALUE_UNKNOWN;
		short refrigerantPressure = VALUE_UNKNOWN;
	} data;

public:

	void init(target::i2c::Peripheral* peripheral, int address) {
		BufferedSlave::init(peripheral, address, NULL, 0, (unsigned char*)&data, sizeof(data));
	}

	virtual void onTimer() {
		target::ADC.CR.setADSTART(1);
	}

	void startTimer() {
		start(100);
	}

	void endOfConversion(int value) {
		if (conv == 0) {
			data.waterPressure = value;
		}
		if (conv == 1) {
			data.refrigerantPressure = value;
		}
		conv++;
	}

	void endOfConversionSequence() {
		conv = 0;
		startTimer();
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
	if (target::ADC.ISR.getEOS()) {
		target::ADC.ISR.setEOS(1);
		ias3.endOfConversionSequence();
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
	target::GPIOA.MODER.setMODER(ADC_CHANNEL_WATER_PRESSURE, 3);
	target::GPIOA.MODER.setMODER(ADC_CHANNEL_REFRIGERANT_PRESSURE, 3);	
	target::RCC.APB2ENR.setADCEN(1);
	target::ADC.CR.setADEN(1);
	target::ADC.CFGR2 = 30 << 1; // CKMODE[1:0] = 01, broken svd definition
	target::ADC.SMPR.setSMPR(7);
	target::ADC.CHSELR.setCHSEL(ADC_CHANNEL_WATER_PRESSURE, 1);
	target::ADC.CHSELR.setCHSEL(ADC_CHANNEL_REFRIGERANT_PRESSURE, 1);
	target::ADC.IER.setEOCIE(1);
	target::ADC.IER.setEOSIE(1);

	target::NVIC.ISER.setSETENA(1 << target::interrupts::External::ADC);
	
	ias3.init(&target::I2C1, 0x70);
	ias3.startTimer();
}
