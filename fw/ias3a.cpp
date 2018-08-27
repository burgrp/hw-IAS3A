const int VALUE_UNKNOWN = -1;

class FrequencyMeter: public genericTimer::Timer {
	int counter;
	int extiNo;
	short* target;
public:
	void init(int extiNo, short* target) {
		this->extiNo = extiNo;
		this->target = target;
		target::EXTI.FTSR.setTR(extiNo, 1);
		target::EXTI.IMR.setMR(extiNo, 1);
		start(100);
	}

	virtual void onTimer() {
		*target = counter;
		counter = 0;
		start(100);
	}

	void handleInterrupt() {
		if (target::EXTI.PR.getPR(extiNo)) {
			counter++;
			target::EXTI.PR.setPR(extiNo, 1);
		}
	}
};

class IAS3{

	struct {
		short waterFlow = VALUE_UNKNOWN;
		short waterPressure = VALUE_UNKNOWN;
		short refrigerantPressure = VALUE_UNKNOWN;
	} data;

	AdcChannel adcChannels[2] = {
		{.pin = 1, .target = &data.waterPressure},
		{.pin = 2, .target = &data.refrigerantPressure}
	};

public:

	i2c::hw::BufferedSlave i2c;
	FrequencyMeter frequencyMeter;
	PeriodicAdc adc;

	void init(target::i2c::Peripheral* peripheral, int address) {
		i2c.init(peripheral, address, NULL, 0, (unsigned char*)&data, sizeof(data));
		frequencyMeter.init(0, &data.waterFlow);
		adc.init(10, adcChannels, sizeof(adcChannels) / sizeof(AdcChannel));		
	}

};

IAS3 ias3;

void interruptHandlerI2C1() {
	ias3.i2c.handleInterrupt();
}

void interruptHandlerADC() {	
	ias3.adc.handleInterrupt();
}

void interruptHandlerEXTI0_1() {
	ias3.frequencyMeter.handleInterrupt();
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
	target::NVIC.ISER.setSETENA(1 << target::interrupts::External::ADC);

	// EXTI for frequency meter	
	target::GPIOA.MODER.setMODER(0, 0);
	target::GPIOA.PUPDR.setPUPDR(0, 1);
	target::SYSCFG.EXTICR1.setEXTI(0, 0);
	target::NVIC.ISER.setSETENA(1 << target::interrupts::External::EXTI0_1);

	// check address switch
	const int addrPin = 3;
	int address;
	target::GPIOA.MODER.setMODER(addrPin, 0);
	target::GPIOA.PUPDR.setPUPDR(addrPin, 1);
	if (target::GPIOA.IDR.getIDR(addrPin) == 0) {
		address = 0x70;
	} else {
		target::GPIOA.PUPDR.setPUPDR(addrPin, 2);
		if (target::GPIOA.IDR.getIDR(addrPin) == 1) {
			address = 0x71;
		} else {
			address = 0x72;
		}
	}

	ias3.init(&target::I2C1, address);	
}
