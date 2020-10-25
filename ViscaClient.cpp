#include "ViscaClient.h"


Visca::Visca(Stream &stream, Stream &debug) : stream(stream), debug(debug) {
	this->stream = stream;
	this->debug = debug;
}

void Visca::begin() {

	this->debug.println("Hello");

	pinMode(13,OUTPUT);
}



void Visca::send_address() {
	this->stream.write((char)(0x80 + 0x10 * this->config_address));
}

void Visca::send_syntax_err() {
	this->send_address();
	this->stream.write("\x60\x20\xFF");
}

void Visca::send_not_executalble_err() {
	this->send_address();
	this->stream.write("\x61\x41\xFF");
}

void Visca::send_ack() {
	this->send_address();
	this->stream.write("\x41\xFF");
}

void Visca::send_complete() {
	this->send_address();
	this->stream.write("\x51\xFF");
}

bool Visca::check_address_byte(char b) {
	return b == '\x88' || b == (char)(0x80 + 0x1 * this->config_address);
}

void Visca::parse_command() {
	this->debug.print("Parsing command: ");
	char tmp[4];
	for (int i = 0; i < this->rx_buffer_ptr + 1; ++i) {
		//this->debug.print(rx_buffer[i], HEX);
		sprintf(tmp, "%02X ", (byte)rx_buffer[i]);
		this->debug.print(tmp);
	}
	this->debug.print(" parsed by ");


	//Commands

	//AddressSet
	if(strncmp(rx_buffer,"\x88\x30\x01\xff",4) == 0) {
		this->send_ack();
		this->debug.println("AddressSet");

		this->send_complete();
	}
	//TODO: IF_Clear

	//CAM_Power
	else if(check_address_byte(rx_buffer[0]) && strncmp(rx_buffer+1,"\x01\x04\x00",3) == 0) {
		this->send_ack();

		this->status_powered = rx_buffer[4] == '\x02';

		this->debug.print("CAM_Power=");
		this->debug.println(this->status_powered?"ON":"OFF");

		this->send_complete();
	}
	//CAM_Zoom
	else if(check_address_byte(rx_buffer[0]) && strncmp(rx_buffer+1,"\x01\x04",2) == 0 && (rx_buffer[3] == '\x07' || rx_buffer[3] == '\x47') ) {
		this->send_ack();

		if (rx_buffer[3] == '\x47') {
			this->status_zoom_position = (rx_buffer[4] << 12) + (rx_buffer[5] << 8) + (rx_buffer[6] << 4) + rx_buffer[7];
			this->debug.print("CAM_Zoom=");
			this->debug.println(status_zoom_position);
		} else {
			if(rx_buffer[4] == 0x02 || rx_buffer[4] == 0x03) { //Force standard commands to fixed variable ones at speed 0
				rx_buffer[4] = rx_buffer[4] << 4;
			}

			if(rx_buffer[4] == 0x00) {
				this->debug.println("CAM_Zoom=STOPPED");
				_zoom_speed=0;
			} else {
				if( (rx_buffer[4] >> 4) == 2) {  //Check tele or wide
					_zoom_speed = (rx_buffer[4] & 0xf) +1;
				} else {
					_zoom_speed = ( (rx_buffer[4] & 0xf)+1 ) *-1;
				}
			}
		}
		this->send_complete();
	}
	//CAM_FocusMode
	else if(check_address_byte(rx_buffer[0]) && strncmp(rx_buffer+1,"\x01\x04\x38",3) == 0) {
		this->send_ack();

		this->status_focus_mode = (FocusMode)(rx_buffer[4]);

		this->debug.print("CAM_FocusMode=");
		this->debug.println(this->status_focus_mode==0x02?"AUTO":"MANUAL");

		this->send_complete();
	}
	//CAM_Focus
	else if(check_address_byte(rx_buffer[0]) && strncmp(rx_buffer+1,"\x01\x04",2) == 0 && (rx_buffer[3] == '\x08' || rx_buffer[3] == '\x18'|| rx_buffer[3] == '\x48') ) {
		this->send_ack();

		if (rx_buffer[3] == '\x48') {
			this->status_focus_position = (rx_buffer[4] << 12) + (rx_buffer[5] << 8) + (rx_buffer[6] << 4) + rx_buffer[7];
			this->debug.print("CAM_Focus=");
			this->debug.println(status_focus_position);
		} else if (rx_buffer[3] == '\x18') {
			if (rx_buffer[4] == '\x01') {
				this->debug.println("NOT IMPLEMENTED (AutoFocus)");
				this->send_syntax_err();
			} else if (rx_buffer[4] == '\x02') {
				this->status_focus_position = 0x4000;
				this->debug.println("CAM_FocusToInfitity=16384 0x4000");
			}

		} else {
			if(rx_buffer[4] == 0x02 || rx_buffer[4] == 0x03) { //Force standard commands to fixed variable ones at speed 0
				rx_buffer[4] = rx_buffer[4] << 4;
			}

			if(rx_buffer[4] == 0x00) {
				this->debug.println("CAM_Zoom=STOPPED");
				_focus_speed=0;
			} else {
				if( (rx_buffer[4] >> 4) == 2) {  //Check tele or wide
					_focus_speed = (rx_buffer[4] & 0xf) +1;
				} else {
					_focus_speed = ( (rx_buffer[4] & 0xf)+1 ) *-1;
				}
			}
		}
		this->send_complete();
	}
	//CAM_ZoomFocus
	else if(check_address_byte(rx_buffer[0]) && strncmp(rx_buffer+1,"\x01\x04\x47",3) == 0 ) {
		this->send_ack();

		this->status_zoom_position = (rx_buffer[4] << 12) + (rx_buffer[5] << 8) + (rx_buffer[6] << 4) + rx_buffer[7];
		this->debug.print("CAM_Zooms=");
		this->debug.println(status_zoom_position);

		this->status_focus_position = (rx_buffer[8] << 12) + (rx_buffer[9] << 8) + (rx_buffer[10] << 4) + rx_buffer[11];
		this->debug.print("CAM_Focus=");
		this->debug.println(status_focus_position);

		this->send_complete();
	}
	//CAM_WB
	else if(check_address_byte(rx_buffer[0]) && strncmp(rx_buffer+1,"\x01\x04\x35",3) == 0) {
		this->send_ack();

		this->status_white_balance_mode = (WhiteBlanceMode) rx_buffer[4];

		this->debug.print("CAM_WB=");
		this->debug.println(this->status_white_balance_mode);

		this->send_complete();
	}
	//CAM_RGain
	else if(check_address_byte(rx_buffer[0]) && strncmp(rx_buffer+1,"\x01\x04",2) == 0 && (rx_buffer[3] == '\x03' || rx_buffer[3] == '\x43') ) {
		this->send_ack();

		if(rx_buffer[3] == '\x03' && rx_buffer[4] == '\x00') this->status_wg_r_gain = 0;
		if(rx_buffer[3] == '\x03' && rx_buffer[4] == '\x02') this->status_wg_r_gain += 1;
		if(rx_buffer[3] == '\x03' && rx_buffer[4] == '\x03') this->status_wg_r_gain -= 1;
		if(rx_buffer[3] == '\x43') {
			this->status_wg_r_gain = (rx_buffer[6]<<4) + rx_buffer[7];
		}

		this->debug.print("CAM_WB_RGAIN=");
		this->debug.println(this->status_wg_r_gain);

		this->send_complete();
	}
	//CAM_Bgain
	else if(check_address_byte(rx_buffer[0]) && strncmp(rx_buffer+1,"\x01\x04",2) == 0 && (rx_buffer[3] == '\x04' || rx_buffer[3] == '\x44') ) {
		this->send_ack();

		if(rx_buffer[3] == '\x04' && rx_buffer[4] == '\x00') this->status_wg_b_gain = 0;
		if(rx_buffer[3] == '\x04' && rx_buffer[4] == '\x02') this->status_wg_b_gain += 1;
		if(rx_buffer[3] == '\x04' && rx_buffer[4] == '\x03') this->status_wg_b_gain -= 1;
		if(rx_buffer[3] == '\x44') {
			this->status_wg_b_gain = (rx_buffer[6]<<4) + rx_buffer[7];
		}

		this->debug.print("CAM_WB_BGAIN=");
		this->debug.println(this->status_wg_b_gain);

		this->send_complete();
	}
	// CAM_AE
	else if(check_address_byte(rx_buffer[0]) && strncmp(rx_buffer+1,"\x01\x04\x39",3) == 0) {
		this->send_ack();

		this->status_autoexposure = (AutoExposureMode) rx_buffer[4];

		this->debug.print("CAM_AE=");
		this->debug.println(this->status_autoexposure);

		this->send_complete();
	}
	//CAM_Shutter
	else if(check_address_byte(rx_buffer[0]) && strncmp(rx_buffer+1,"\x01\x04",2) == 0 && (rx_buffer[3] == '\x0A' || rx_buffer[3] == '\x4A')) {
		this->send_ack();


		if( rx_buffer[3] == '\x04' && rx_buffer[4] == '\x00' ) this->status_shutter = ShutterSpeed::SS_1_OVER_60;
		if( rx_buffer[3] == '\x04' && rx_buffer[4] == '\x02' ) this->status_shutter = (ShutterSpeed) (this->status_shutter + 1);
		if( rx_buffer[3] == '\x04' && rx_buffer[4] == '\x03' ) this->status_shutter = (ShutterSpeed) (this->status_shutter - 1);

		if( rx_buffer[3] == '\x4A' ) {
			this->status_shutter = (ShutterSpeed) rx_buffer[4];
		}

		this->debug.print("CAM_SHUTTER=");
		this->debug.println(this->status_shutter);

		this->send_complete();
	}
	//CAM_Iris
	else if(check_address_byte(rx_buffer[0]) && strncmp(rx_buffer+1,"\x01\x04",2) == 0 && (rx_buffer[3] == '\x0B' || rx_buffer[3] == '\x4B')) {
		this->send_ack();


		if( rx_buffer[3] == '\x04' && rx_buffer[4] == '\x00' ) this->status_iris = Iris::IR_F32;
		if( rx_buffer[3] == '\x04' && rx_buffer[4] == '\x02' ) this->status_iris = (Iris) (this->status_shutter + 1);
		if( rx_buffer[3] == '\x04' && rx_buffer[4] == '\x03' ) this->status_iris = (Iris) (this->status_shutter - 1);

		if( rx_buffer[3] == '\x4A' ) {
			this->status_iris = (Iris) rx_buffer[4];
		}

		this->debug.print("CAM_IRIS=");
		this->debug.println(this->status_iris);

		this->send_complete();
	}
	//CAM_Gain
	else if(check_address_byte(rx_buffer[0]) && strncmp(rx_buffer+1,"\x01\x04",2) == 0 && (rx_buffer[3] == '\x0B' || rx_buffer[3] == '\x4B')) {
		this->send_ack();


		if( rx_buffer[3] == '\x04' && rx_buffer[4] == '\x00' ) this->status_gain = 0;
		if( rx_buffer[3] == '\x04' && rx_buffer[4] == '\x02' ) this->status_gain += 2;
		if( rx_buffer[3] == '\x04' && rx_buffer[4] == '\x03' ) this->status_gain -= 2;

		if( rx_buffer[3] == '\x4A' ) {
			this->status_gain += rx_buffer[4] *2;
		}

		if(this->status_gain >28) this->status_gain = 28;
		if(this->status_gain <0) this->status_gain = 0;

		this->debug.print("CAM_GAIN=");
		this->debug.println(this->status_gain);

		this->send_complete();
	}

	//CAM_Bright
	else if(check_address_byte(rx_buffer[0]) && strncmp(rx_buffer+1,"\x01\x04",2) == 0 && (rx_buffer[3] == '\x0d' || strncmp(rx_buffer+3,"\x4d\x00\x00",3) == 0) ) {
		this->send_ack();

		if(rx_buffer[3] == '\x0d' && rx_buffer[4] =='\x00') {
			status_brightness = 128;
			this->debug.print("CAM_Bright_Reset=128");
		}

		if(rx_buffer[3] == '\x0d' && rx_buffer[4] =='\x02') {
			this->status_brightness += 8;
			this->status_brightness = status_brightness> 255 ? 255 :  this->status_brightness;
			this->debug.print("CAM_Bright_Up=");
			this->debug.println(status_brightness);
		}

		if(rx_buffer[3] == '\x0d' && rx_buffer[4] =='\x03') {
			this->status_brightness -= 8;
			this->status_brightness = this->status_brightness< 0 ? 0 : this->status_brightness;
			this->debug.print("CAM_Bright_Down=");
			this->debug.println(status_brightness);
		}

		if(rx_buffer[3] == '\x4d' && rx_buffer[4] =='\x00' && rx_buffer[5] =='\x00') {
			this->status_brightness = (rx_buffer[6] << 4) + rx_buffer[7];
			this->debug.print("CAM_Bright_Set=");
			this->debug.println(this->status_brightness);
		}

		this->send_complete();
	}
	//CAM_ExpComp
	else if(check_address_byte(rx_buffer[0]) && strncmp(rx_buffer+1,"\x01\x04",2) == 0 && (rx_buffer[3] == '\x3e' || rx_buffer[3] == '\x0e' || rx_buffer[3] == '\x4e') ) {
		this->send_ack();

		if(rx_buffer[3] == '\x3e') {
			this->status_exp_comp_en = rx_buffer[4] == '\x02';
			this->debug.print("CAM_EXPCOMP=");
			this->debug.println(this->status_exp_comp_en?"ON":"OFF");
		}

		if(rx_buffer[3] == '\x0e' && rx_buffer[4] =='\x00') {
			this->status_exp_comp = 7;
			this->debug.print("CAM_EXPCOMP_RESET");
			this->debug.println(this->status_exp_comp);
		}

		if(rx_buffer[3] == '\x0e' && rx_buffer[4] =='\x03') {
			this->status_exp_comp -= 1;
			this->status_exp_comp = this->status_exp_comp< 0 ? 0 :  this->status_exp_comp;
			this->debug.print("CAM_EXPCOMP_DOWN=");
			this->debug.println(status_brightness);
		}

		if(rx_buffer[3] == '\x0e' && rx_buffer[4] =='\x02') {
			this->status_exp_comp += 1;
			this->status_exp_comp = this->status_exp_comp> 0x0e ? 0x0e :  this->status_exp_comp;
			this->debug.print("CAM_EXPCOMP_UP=");
			this->debug.println(this->status_exp_comp);
		}

		if(rx_buffer[3] == '\x4e' && rx_buffer[4] =='\x00' && rx_buffer[5] =='\x00') {
			this->status_exp_comp = (rx_buffer[6] << 4) + rx_buffer[7];
			this->debug.print("CAM_EXPCOMP=");
			this->debug.println(this->status_exp_comp);
		}

		this->send_complete();
	}
	//CAM_BackLight
	else if(check_address_byte(rx_buffer[0]) && strncmp(rx_buffer+1,"\x01\x04\x33",3) == 0) {
		this->send_ack();

		this->status_backlight = rx_buffer[4]==0x2;

		this->debug.print("CAM_BackLight=");
		this->debug.println(this->status_backlight);

		this->send_complete();
	}
	//CAM_Aperture
	else if(check_address_byte(rx_buffer[0]) && strncmp(rx_buffer+1,"\x01\x04",2) == 0 && (rx_buffer[3] == '\x02' || rx_buffer[3] == '\x42') ) {
		this->send_ack();

		if (rx_buffer[3] == '\x0e' && rx_buffer[4] == '\x00') {
			this->status_exp_comp = 7;
			this->debug.print("CAM_Aperture_Reset=");
			this->debug.println(this->status_aperture);
		}

		if (rx_buffer[3] == '\x0e' && rx_buffer[4] == '\x03') {
			this->status_aperture -= 1;
			this->status_aperture = this->status_aperture < 0 ? 0 : this->status_aperture;
			this->debug.print("CAM_Aperture_DOWN=");
			this->debug.println(this->status_aperture);
		}

		if (rx_buffer[3] == '\x0e' && rx_buffer[4] == '\x02') {
			this->status_aperture += 1;
			this->status_aperture = this->status_aperture > 0x04 ? 0x04 : this->status_aperture;
			this->debug.print("CAM_Aperture_UP=");
			this->debug.println(this->status_aperture);
		}

		if (rx_buffer[3] == '\x4e' && rx_buffer[4] == '\x00' && rx_buffer[5] == '\x00') {
			this->status_aperture = (rx_buffer[6] << 4) + rx_buffer[7];
			this->debug.print("CAM_Aperture=");
			this->debug.println(this->status_aperture);
		}
	}
	//TODO: CAM_Memory
	else if(check_address_byte(rx_buffer[0]) && strncmp(rx_buffer+1,"\x01\x04\x3f",3) == 0) {
		send_syntax_err();
	}
	//CAM_LR_Reverse
	else if(check_address_byte(rx_buffer[0]) && strncmp(rx_buffer+1,"\x01\x04\x61",3) == 0) {
		this->send_ack();

		this->status_lr_reverse = rx_buffer[4] == '\x02';

		this->debug.print("CAM_LR_Reverse=");
		this->debug.println(this->status_lr_reverse?"INVERTED":"NORMAL");

		this->send_complete();
	}
	//CAM_PictureFlip
	else if(check_address_byte(rx_buffer[0]) && strncmp(rx_buffer+1,"\x01\x04\x66",3) == 0) {
		this->send_ack();

		this->status_ud_reverse = rx_buffer[4] == '\x02';

		this->debug.print("CAM_PictureFlip=");
		this->debug.println(this->status_ud_reverse?"INVERTED":"NORMAL");

		this->send_complete();
	}
	//CAM_MountMode
	else if(check_address_byte(rx_buffer[0]) && strncmp(rx_buffer+1,"\x01\x04\xA4",3) == 0) {
		this->send_ack();

		this->status_mount_mode = (MounteMode)rx_buffer[4];

		this->debug.print("CAM_MountMode=");
		this->debug.println(this->status_mount_mode==0x02?"UP":"DOWN");

		this->send_complete();
	}
	//CAM_ColorGain
	else if(check_address_byte(rx_buffer[0]) && strncmp(rx_buffer+1,"\x01\x04\x49",3) == 0) {
		this->send_ack();

		this->status_color_gain = rx_buffer[7];

		this->debug.print("CAM_ColorGain=");
		this->debug.println(this->status_color_gain);

		this->send_complete();
	}
	//TODO: CAM_2D_Noise_reduction
	else if(check_address_byte(rx_buffer[0]) && strncmp(rx_buffer+1,"\x01\x04\x53",3) == 0) {
		send_syntax_err();
	}
	//TODO: CAM_3D_Noise_reduction
	else if(check_address_byte(rx_buffer[0]) && strncmp(rx_buffer+1,"\x01\x04\x54",3) == 0) {
		send_syntax_err();
	}
	//FLICK
	else if(check_address_byte(rx_buffer[0]) && strncmp(rx_buffer+1,"\x01\x04\x23",3) == 0) {
		this->send_ack();

		this->status_flick = (PowerFreq)rx_buffer[4];

		this->debug.print("FLICK=");
		this->debug.println(this->status_flick);

		this->send_complete();
		send_syntax_err();
	}
	//Freeze
	else if(check_address_byte(rx_buffer[0]) && strncmp(rx_buffer+1,"\x01\x04\x62",3) == 0) {
		this->send_ack();
		if( (rx_buffer[4] & 0xf0) == 0x20) {
			this->debug.println("FreezePreset (NOT IMPLEMENTED)");
			send_syntax_err();
		} else {
			this->status_freeze = rx_buffer[4] == 0x02;

			this->debug.print("Freeze=");
			this->debug.println(this->status_flick);

			this->send_complete();
		}
	}
	//TODO: VideoSystemSet
	else if(check_address_byte(rx_buffer[0]) && strncmp(rx_buffer+1,"\x01\x06\x35\x00",4) == 0) {
		send_syntax_err();
	}
	//TODO: CAM_IDWrite
	else if(check_address_byte(rx_buffer[0]) && strncmp(rx_buffer+1,"\x01\x04\x22",3) == 0) {
		send_syntax_err();
	}
	//TODO: SYS_Menu
	else if(check_address_byte(rx_buffer[0]) && strncmp(rx_buffer+1,"\x01\x04\x53",3) == 0) {
		send_syntax_err();
	}
	//TODO: IR_Transfer
	else if(check_address_byte(rx_buffer[0]) && strncmp(rx_buffer+1,"\x01\x06\x1A",3) == 0) {
		send_syntax_err();
	}
	//TODO: IR_Receive
	else if(check_address_byte(rx_buffer[0]) && strncmp(rx_buffer+1,"\x01\x06\x08",3) == 0) {
		send_syntax_err();
	}
	//TODO: IR_ReceiveReturn
	else if(check_address_byte(rx_buffer[0]) && strncmp(rx_buffer+1,"\x01\x7d\x01",3) == 0) {
		send_syntax_err();
	}
	//TODO: Pan_tiltDrive

	//TODO: Pan-tiltLimitSet



	//Inquiries

	//CAM_PowerInq
	else if(check_address_byte(rx_buffer[0]) && strncmp(rx_buffer+1,"\x09\x04\x00\xff",4) == 0) {
		this->debug.println("CAM_PowerInq");
		this->send_address();

		if(this->status_powered)
			this->stream.write("\x50\x02\xFF");
		else
			this->stream.write("\x50\x03\xFF");
	}
	//TODO: CAM_ZoomPosInq

	//TODO: CAM_FocusModeInq

	//TODO: CAM_FocusPosInq
	else if(check_address_byte(rx_buffer[0]) && strncmp(rx_buffer+1,"\x09\x04\x38\xff",4) == 0) {
		this->debug.println("CAM_FocusPosInq");
		this->send_address();

		this->stream.write("\x50");
		this->stream.write(this->status_focus_mode);
		this->stream.write("\xFF");
	}
	//CAM_WBModeInq
	else if(check_address_byte(rx_buffer[0]) && strncmp(rx_buffer+1,"\x09\x04\x35\xff",4) == 0) {
		this->debug.println("CAM_WBModeInq");
		this->send_address();

		this->stream.write("\x50");
		this->stream.write(this->status_white_balance_mode);
		this->stream.write("\xFF");
	}

	//TODO: CAM_RGainInq

	//TODO: CAM_BGainInq

	//CAM_AEModeInq
	else if(check_address_byte(rx_buffer[0]) && strncmp(rx_buffer+1,"\x09\x04\x39\xff",4) == 0) {
		this->debug.println("CAM_AEModeInq");
		this->send_address();

		this->stream.write("\x50");
		this->stream.write(this->status_autoexposure);
		this->stream.write("\xFF");
	}
	//TODO: CAM_BGainInq

	//TODO: CAM_ShutterPosInq

	//CAM_IrisPosInq
	else if(check_address_byte(rx_buffer[0]) && strncmp(rx_buffer+1,"\x09\x04\x4b\xff",4) == 0) {
		this->debug.println("CAM_IrisPosInq");
		send_syntax_err();
		/*
		this->send_address();

		this->stream.write("\x50");
		this->stream.write("\x00");
		this->stream.write("\x00");

		//TODO: Implement proper values
		this->stream.write("\x00");
		this->stream.write("\x01");

		this->stream.write("\xFF");*/
	}

	//CAM_GainPosiInq
	else if(check_address_byte(rx_buffer[0]) && strncmp(rx_buffer+1,"\x09\x04\x4c\xff",4) == 0) {
		this->debug.println("CAM_GainPosiInq");
		send_syntax_err();
		/*
		this->send_address();

		this->stream.write("\x50");
		this->stream.write("\x00");
		this->stream.write("\x00");

		//TODO: Implement proper values
		this->stream.write("\x00");
		this->stream.write("\x0A");

		this->stream.write("\xFF");*/
	}
	//CAM_BrightPosiInq
	else if(check_address_byte(rx_buffer[0]) && strncmp(rx_buffer+1,"\x09\x04\x4d\xff",4) == 0) {
		this->debug.println("CAM_BrightPosiInq");
		send_syntax_err();
		/*
		this->send_address();

		this->stream.write("\x50");
		this->stream.write("\x00");
		this->stream.write("\x00");

		//TODO: Implement proper values
		this->stream.write("\x01");
		this->stream.write("\x08");

		this->stream.write("\xFF");*/
	}

	//TODO: CAM_ExpCompModeInq

	//TODO: CAM_ExpCompPosInq

	//TODO: CAM_ApertureInq

	//TODO: CAM_MemoryInq

	//TODO: SYS_MenuModeInq

	//TODO: CAM_LR_ReverseInq

	//TODO: CAM_PictureFlipInq

	//TODO: CAM_IDInq

	//TODO: CAM_VersionInq

	//TODO: VideoSystemInq

	//TODO: IR_Transfer

	//TODO: IR_Receive

	//TODO: IR_ReceiveReturn

	//TODO: Pan-tiltMaxSpeedInq

	//TODO: Pan-tiltPosInq

	//TODO: CAM_VersionInq

	//TODO: CAM_VersionInq

	//CAM_BackLightInq
	else if(check_address_byte(rx_buffer[0]) && strncmp(rx_buffer+1,"\x09\x04\x33\xff",4) == 0) {
		this->debug.println("Subset - CAM_BackLightInq");
		this->send_address();

		if(this->status_powered)
			this->stream.write("\x50\x02\xFF");
		else
			this->stream.write("\x50\x03\xFF");
	}

	else {
		this->debug.println("NOT IMPLEMENTED");
		send_syntax_err();
	}
}




void Visca::loop() {
	while(this->stream.available()) {
		rx_buffer[this->rx_buffer_ptr] = (char) this->stream.read();


		if(rx_buffer[this->rx_buffer_ptr] == '\xff') {
			this->parse_command();

			memset(rx_buffer, 0, sizeof(rx_buffer));
			this->rx_buffer_ptr=-1;
		}

		if(this->rx_buffer_ptr >= RX_BUFFER_SIZE) {
			memset(rx_buffer, 0, sizeof(rx_buffer));
			this->rx_buffer_ptr=-1;
		}

		this->rx_buffer_ptr++;
	}

	status_zoom_position += _zoom_speed * 5;
	if(status_zoom_position<0) status_zoom_position=0;
	if(status_zoom_position>16384) status_zoom_position=16384;

	status_focus_position += _focus_speed * 2;
	if(status_focus_position<0) status_focus_position=0;
	if(status_focus_position>16384) status_focus_position=16384;

	if(_zoom_speed!=0) {
		this->debug.print("CAM_Zoom=");
		this->debug.print(status_zoom_position);
		this->debug.print(" 0x");
		this->debug.println(status_zoom_position,HEX);
	}

	if(_focus_speed!=0) {
		this->debug.print("CAM_Focus=");
		this->debug.print(status_focus_position);
		this->debug.print(" 0x");
		this->debug.println(status_focus_position,HEX);
	}
}


