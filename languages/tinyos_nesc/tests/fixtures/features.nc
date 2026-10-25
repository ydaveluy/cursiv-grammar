module FeaturesC {
}
implementation {
	typedef nx_struct radio_msg { uint8_t x; } radio_msg_t;

	task void loops() {
		if (busy)
			signal Done.fired();
		for (i = 0; i < 10; i++)
			total = total + i;
		while (busy)
			call Leds.led0Toggle();
		atomic {
			state = 0;
			busy = FALSE;
		}
	}

	enum { A, B, C = 5 };

	command error_t Foo.bar(uint8_t x) {
		switch (x) {
			case 1:
				return SUCCESS;
			case 2:
				return FAIL;
			default:
				break;
		}
		return SUCCESS;
	}

	void helper() {
		return;
	}
}
