struct AdcChannel {
	int pin;
	short* target;	
};

class PeriodicAdc: public genericTimer::Timer {
public:
	AdcChannel* adcChannels;
	int adcChannelsCount;
	int adcChannel;
    int periodTenMs;

	void init(int periodTenMs, AdcChannel* adcChannels, int adcChannelsCount) {
        this->periodTenMs = periodTenMs;
		this->adcChannels = adcChannels;
		this->adcChannelsCount = adcChannelsCount;
		for (int c = 0; c < adcChannelsCount; c++) {
			target::GPIOA.MODER.setMODER(adcChannels[c].pin, 3);
		}

        target::ADC.CR.setADEN(1);
        target::ADC.CFGR2 = 30 << 1; // CKMODE[1:0] = 01, broken svd definition
        target::ADC.SMPR.setSMPR(7);
        target::ADC.IER.setEOCIE(1);

        startAdcConversion();
	}

	virtual void onTimer() {
		startAdcConversion();		
	}

	void startAdcConversion() {
		target::ADC.CHSELR.setCHSEL(1 << adcChannels[adcChannel].pin);
		target::ADC.CR.setADSTART(1);
	}

    void handleInterrupt() {
        if (target::ADC.ISR.getEOC()) {
            *adcChannels[adcChannel].target = target::ADC.DR.getDATA();
            adcChannel++;
            if (adcChannel >= adcChannelsCount) {
                adcChannel = 0;
            }
            start(periodTenMs);
    	}
    }

};
