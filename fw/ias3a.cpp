const int VALUE_UNKNOWN = -1;

const int sampleCount = 512;

class SMT160 {	
	int extiNo;
	short* target;
	volatile target::tim_16_17::Peripheral* timer;
	int fall = 0;
	short cnts[sampleCount];
	short falls[sampleCount];
	int sampleIndex = 0;
public:
	void init(int extiNo, short* target, volatile target::tim_16_17::Peripheral* timer) {
		this->extiNo = extiNo;
		this->target = target;
		target::EXTI.FTSR.setTR(extiNo, 1);
		target::EXTI.RTSR.setTR(extiNo, 1);
		target::EXTI.IMR.setMR(extiNo, 1);
		this->timer = timer;			
		timer->ARR.setARR(0xFFFF);
		timer->CR1.setCEN(1);
	}

	void handleInterrupt() {
		if (target::EXTI.PR.getPR(extiNo)) {
			bool rising = target::GPIOA.IDR.getIDR(0);
			if (rising) {
				int cnt = timer->CNT;
				timer->EGR.setUG(1);
				cnts[sampleIndex] = cnt;
				falls[sampleIndex] = fall;
				sampleIndex++;
				if (sampleIndex == sampleCount) {
					sampleIndex = 0;
					int sumCnt = 0;
					int sumFall = 0;
					for (int c = 0; c < sampleCount; c++) {
						sumCnt += cnts[c];
						sumFall += falls[c];
					}
					int avgCnt = sumCnt / sampleCount;
					int avgFall = sumFall / sampleCount;
					
					int v = 0x10000 * avgFall / avgCnt;
					*target = v;
				}

			} else {
				fall = timer->CNT;				
			}
			target::EXTI.PR.setPR(extiNo, 1);
		}
	}

	int get() {
	}
};

class IAS3{

	iwdg::Driver iwdg;

	struct {
		short refrigerantTemp = VALUE_UNKNOWN;
		short waterPressure = VALUE_UNKNOWN;
		short refrigerantPressure = VALUE_UNKNOWN;
	} data;

	AdcChannel adcChannels[2] = {
		{.pin = 1, .target = &data.waterPressure},
		{.pin = 2, .target = &data.refrigerantPressure}
	};

public:

	i2c::hw::BufferedSlave i2c;
	SMT160 smt160;
	PeriodicAdc adc;

	void init(target::i2c::Peripheral* peripheral, int address, volatile target::tim_16_17::Peripheral* smtimer) {
		iwdg.init();
		i2c.init(peripheral, address, NULL, 0, (unsigned char*)&data, sizeof(data));
		smt160.init(0, &data.refrigerantTemp, smtimer);
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
	ias3.smt160.handleInterrupt();
}

void initApplication() {

	target::RCC.AHBENR.setIOPAEN(true);
	
	// I2C peripheral
	target::GPIOA.AFRH.setAFRH(9, 4);
	target::GPIOA.AFRH.setAFRH(10, 4);
	target::GPIOA.MODER.setMODER(9, 2);
	target::GPIOA.MODER.setMODER(10, 2);
	target::RCC.APB1ENR.setC_EN(1, 1);
	//target::NVIC.IPR5.setPRI_203(3);
	//target::NVIC.IPR5 = 0xFFFFFFFF;
	target::NVIC.ISER.setSETENA(1 << target::interrupts::External::I2C1);

	// ADC peripheral
	target::RCC.APB2ENR.setADCEN(1);
	//target::NVIC.IPR3.setPRI_120(3);
	//target::NVIC.IPR3 = 0xFFFFFFFF;
	target::NVIC.ISER.setSETENA(1 << target::interrupts::External::ADC);

	target::RCC.APB2ENR.setTIM16EN(1);
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
	for (volatile int c = 0; c < 1000; c++);
	if (target::GPIOA.IDR.getIDR(addrPin) == 0) {
		address = 0x70;
	} else {
		target::GPIOA.PUPDR.setPUPDR(addrPin, 2);
		for (volatile int c = 0; c < 1000; c++);
		if (target::GPIOA.IDR.getIDR(addrPin) == 1) {
			address = 0x71;
		} else {
			address = 0x72;
		}
	}

	ias3.init(&target::I2C1, address, &target::TIM16);
}
