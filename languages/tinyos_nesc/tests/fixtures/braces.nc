module BracesC {
}
implementation {
	typedef nx_struct radio_msg { uint8_t x; } radio_msg_t;

	task void loops() {
		atomic {
			state = 0;
		}
	}

	void helper() {
		return;
	}

	void deref() {
		*p = 1;
	}
}
