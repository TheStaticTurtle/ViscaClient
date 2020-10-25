
#ifndef VISCACLIENT_h
#define VISCACLIENT_h

#include "Arduino.h"
#include "Stream.h"

#define RX_BUFFER_SIZE 128

enum AutoExposureMode {
	AE_FULL_AUTO = 0x00,
	AE_MANUAL = 0x03,
	AE_SHUTTER_PRIORITY = 0x0A,
	AE_IRIS_PRIORITY = 0x0B,
	AE_BRIGHT = 0x0D,
};

enum FocusMode {
	F_AUTO = 0x02,
	F_MANUAL = 0x03,
};

enum WhiteBlanceMode {
	WB_AUTO = 0x00,
	WB_INDOOR = 0x01,
	WB_OUTDOOR = 0x02,
	WB_ONEPUSH = 0x03,
	WB_ATW = 0x04,
	WB_MANUAL = 0x05,
	WB_OUTDOOR_AUTO = 0x06,
	WB_SODIUM_LAMP = 0x07,
	WB_SODIUM_LAMP_AUTO = 0x08,
};

enum ShutterSpeed {
	SS_1_OVER_10000 = 21,
	SS_1_OVER_6000  = 20,
	SS_1_OVER_4000  = 19,
	SS_1_OVER_3000  = 18,
	SS_1_OVER_2000  = 17,
	SS_1_OVER_1500  = 16,
	SS_1_OVER_1000  = 15,
	SS_1_OVER_725   = 14,
	SS_1_OVER_500   = 13,
	SS_1_OVER_350   = 12,
	SS_1_OVER_250   = 11,
	SS_1_OVER_180   = 10,
	SS_1_OVER_125   = 9,
	SS_1_OVER_100   = 8,
	SS_1_OVER_90    = 7,
	SS_1_OVER_60    = 6,
	SS_1_OVER_30    = 5,
	SS_1_OVER_15    = 4,
	SS_1_OVER_8     = 3,
	SS_1_OVER_4     = 2,
	SS_1_OVER_2     = 1,
	SS_1            = 0,
};

enum Iris {
	IR_CLOSED = 0,
	IR_F32    = 1,
	IR_F28    = 2,
	IR_F24    = 3,
	IR_F22    = 4,
	IR_F18    = 5,
	IR_F14    = 6,
	IR_F11    = 7,
	IR_F9_6   = 8,
	IR_F6_8   = 9,
	IR_F5_6   = 10,
	IR_F4_8   = 11,
	IR_F4_0   = 12,
	IR_F3_4   = 13,
	IR_F2_8   = 14,
	IR_F2_4   = 15,
	IR_F2_0   = 16,
	IR_F1_8   = 17,
};

enum MounteMode {
	MT_UP = 0x2,
	MT_DOWN = 0x3,
};

enum PowerFreq {
	F_50HZ = 0x01,
	F_60HZ = 0x02
};

class Visca
{
	public:
		Visca(Stream& s,Stream& d);
		void loop();
		void begin();

	private:
		Stream& stream;
		Stream& debug;

		void parse_command();

		byte rx_buffer_ptr = 0;
		char rx_buffer[RX_BUFFER_SIZE];

		bool check_address_byte(char b);

		void send_address();
		void send_ack();
		void send_complete();
		void send_syntax_err();
		void send_not_executalble_err();

		byte config_address = 1;

		bool status_powered = false;
		AutoExposureMode status_autoexposure = AutoExposureMode::AE_FULL_AUTO;
		FocusMode status_focus_mode = FocusMode::F_MANUAL;
		WhiteBlanceMode status_white_balance_mode = WhiteBlanceMode::WB_AUTO;

		int8_t _zoom_speed = 0;
		short status_zoom_position = 0;
		int8_t _focus_speed = 0;
		short status_focus_position = 0;

		short status_brightness = 128;
		byte status_wg_r_gain = 0;
		byte status_wg_b_gain = 0;

		ShutterSpeed status_shutter = ShutterSpeed::SS_1_OVER_60;
		Iris status_iris = Iris::IR_F32;
		int8_t status_gain = 0;

		bool status_exp_comp_en = false;
		int8_t status_exp_comp    = 7;

		int8_t status_aperture = 0;

		bool status_lr_reverse = false;
		bool status_ud_reverse = false;

		MounteMode status_mount_mode = MounteMode::MT_UP;

		int8_t status_color_gain = 0;

		PowerFreq status_flick = PowerFreq::F_50HZ;

		bool status_freeze = false;

		bool status_backlight = false;
};

#endif