/************************************************************
************************************************************/
#pragma once

/************************************************************
************************************************************/
#include "stdio.h"

#include "ofMain.h"
#include "ofxGui.h"

#include "ofxOsc_BiDirection.h"

/************************************************************
************************************************************/
#define ERROR_MSG(); printf("Error in %s:%d\n", __FILE__, __LINE__);

/************************************************************
************************************************************/
enum{
	WINDOW_WIDTH	= 1600,	// 切れの良い解像度でないと、ofSaveScreen()での画面保存が上手く行かなかった(真っ暗な画面が保存されるだけ).
	WINDOW_HEIGHT	= 840,
};

enum{
	FBO_FFT_WIDTH		= 1600,
	FBO_FFT_HEIGHT		= 120,
	
	FBO_TIME_WIDTH		= 530,
	FBO_TIME_HEIGHT		= 120,
};

enum{
	BUF_SIZE_S = 500,
	BUF_SIZE_M = 1000,
	BUF_SIZE_L = 6000,
};

enum{
	AUDIO_BUF_SIZE = 1024,
	
	AUDIO_BUFFERS = 2,
	AUDIO_SAMPLERATE = 44100,
};

enum{
	GRAPH_BAR_WIDTH__FFT_GAIN = 1,
	GRAPH_BAR_SPACE__FFT_GAIN = 3,
};

enum{
	AUDIO_CH_L,
	AUDIO_CH_R,
	
	NUM_AUDIO_CHS,
};

enum{
	NUM_FREQ_ZONES = 3,
};

enum{
	TIMEDATA__RAW,
	TIMEDATA__VEL,
	TIMEDATA__ACCEL,
	TIMEDATA__JERK,
	TIMEDATA__STATE,
	
	NUM_TIMEDATA,
};
	
enum{
	OSC_TARGET__VIDEO,
	OSC_TARGET__CLAPON,
	OSC_TARGET__STROBE,
	
	NUM_OSC_TARGET,
};


/************************************************************
************************************************************/

/**************************************************
Derivation
	class MyClass : private Noncopyable {
	private:
	public:
	};
**************************************************/
class Noncopyable{
protected:
	Noncopyable() {}
	~Noncopyable() {}

private:
	void operator =(const Noncopyable& src);
	Noncopyable(const Noncopyable& src);
};


/**************************************************
**************************************************/
class GUI_GLOBAL{
private:
	/****************************************
	****************************************/
	struct GUI_PARAM_SET_FLOAT{
		float initial;
		float from;
		float to;
		
		GUI_PARAM_SET_FLOAT(double _initial, double _from, double _to)
		: initial(_initial), from(_from), to(_to)
		{
		}
		
		void set(float _initial, float _from, float _to){
			initial = _initial;
			from = _from;
			to = _to;
		}
	};
	
	void ReadAndSet_PreSet__Scale();
	void ReadAndSet_PreSet__Param();
	void ReadAndSet_init_from_to(GUI_PARAM_SET_FLOAT& ParamSet, FILE* fp, char* buf);
	
public:
	/****************************************
	****************************************/
	GUI_GLOBAL();
	void setup(string GuiName, string FileName = "gui.xml", float x = 10, float y = 10);
	
	/********************
	********************/
	GUI_PARAM_SET_FLOAT Paramset__GraphScale_FFTGain_Monitor = GUI_PARAM_SET_FLOAT(6.0e-05, 0.0, 12.0e-5);
	GUI_PARAM_SET_FLOAT Paramset__GraphScale_FFTGain_Monitor_H = GUI_PARAM_SET_FLOAT(1.2e-6, 0, 2.4e-6);
	GUI_PARAM_SET_FLOAT Paramset__GraphScale_FFTGain_Diff = GUI_PARAM_SET_FLOAT(6.0e-05, 0, 12.0e-5);
	GUI_PARAM_SET_FLOAT Paramset__GraphScale_FFTGain_Diff_H = GUI_PARAM_SET_FLOAT(1.2e-6, 0, 2.4e-6);
	GUI_PARAM_SET_FLOAT Paramset__GraphScale_Time_Raw[NUM_FREQ_ZONES] = {
		GUI_PARAM_SET_FLOAT(6.0e-5, 0, 12.0e-5),
		GUI_PARAM_SET_FLOAT(4.6e-6, 0, 9.2e-6),
		GUI_PARAM_SET_FLOAT(1.2e-6, 0, 2.4e-6),
	};
	GUI_PARAM_SET_FLOAT Paramset__GraphScale_Time_Vel[NUM_FREQ_ZONES] = {
		GUI_PARAM_SET_FLOAT(1.5e-3, 0, 3.0e-3),
		GUI_PARAM_SET_FLOAT(1.2e-4, 0, 2.4e-4),
		GUI_PARAM_SET_FLOAT(2.7e-5, 0, 5.4e-5),
	};
	GUI_PARAM_SET_FLOAT Paramset__GraphScale_Time_Acc[NUM_FREQ_ZONES] = {
		GUI_PARAM_SET_FLOAT(4.0e-2, 0, 8.0e-2),
		GUI_PARAM_SET_FLOAT(3.0e-3, 0, 6.0e-3),
		GUI_PARAM_SET_FLOAT(7.0e-4, 0, 14e-4),
	};
	GUI_PARAM_SET_FLOAT Paramset__GraphScale_Time_Jerk[NUM_FREQ_ZONES] = {
		GUI_PARAM_SET_FLOAT(1.8e-1, 0, 3.6e-1),
		GUI_PARAM_SET_FLOAT(1.3e-2, 0, 2.6e-2),
		GUI_PARAM_SET_FLOAT(3.5e-3, 0, 7.0e-3),
	};
	GUI_PARAM_SET_FLOAT Paramset__thresh_raw_L[NUM_FREQ_ZONES] = {
		GUI_PARAM_SET_FLOAT(1.2e-05, 0, 2.4e-05),
		GUI_PARAM_SET_FLOAT(0.9e-6, 0, 1.8e-6),
		GUI_PARAM_SET_FLOAT(0.24e-6, 0, 0.48e-6),
	};
	GUI_PARAM_SET_FLOAT Paramset__thresh_vel_H[NUM_FREQ_ZONES] = {
		GUI_PARAM_SET_FLOAT(0.5e-3, 0, 1.0e-3),
		GUI_PARAM_SET_FLOAT(0.4e-4, 0, 0.8e-4),
		GUI_PARAM_SET_FLOAT(0.9e-5, 0, 1.8e-5),
	};
	GUI_PARAM_SET_FLOAT Paramset__thresh_acc_H[NUM_FREQ_ZONES] = {
		GUI_PARAM_SET_FLOAT(1.3e-2, 0, 2.6e-2),
		GUI_PARAM_SET_FLOAT(1.0e-3, 0, 2.0e-3),
		GUI_PARAM_SET_FLOAT(2.3e-4, 0, 4.6e-4),
	};
	GUI_PARAM_SET_FLOAT Paramset__Hys_0cross_vel[NUM_FREQ_ZONES] = {
		GUI_PARAM_SET_FLOAT(0, 0, 1.0e-5),
		GUI_PARAM_SET_FLOAT(0, 0, 1.0e-5),
		GUI_PARAM_SET_FLOAT(0, 0, 1.0e-5),
	};
	GUI_PARAM_SET_FLOAT Paramset__Hys_0cross_acc[NUM_FREQ_ZONES] = {
		GUI_PARAM_SET_FLOAT(0, 0, 1.0e-5),
		GUI_PARAM_SET_FLOAT(0, 0, 1.0e-5),
		GUI_PARAM_SET_FLOAT(0, 0, 1.0e-5),
	};
	GUI_PARAM_SET_FLOAT Paramset__Hys_0cross_jerk[NUM_FREQ_ZONES] = {
		GUI_PARAM_SET_FLOAT(0, 0, 1.0e-5),
		GUI_PARAM_SET_FLOAT(0, 0, 1.0e-5),
		GUI_PARAM_SET_FLOAT(0, 0, 1.0e-5),
	};
	GUI_PARAM_SET_FLOAT Paramset__MaxError_0cross_vel[NUM_FREQ_ZONES] = {
		GUI_PARAM_SET_FLOAT(0, 0, 20),
		GUI_PARAM_SET_FLOAT(0, 0, 20),
		GUI_PARAM_SET_FLOAT(0, 0, 20),
	};
	GUI_PARAM_SET_FLOAT Paramset__MaxError_0cross_acc[NUM_FREQ_ZONES] = {
		GUI_PARAM_SET_FLOAT(0, 0, 20),
		GUI_PARAM_SET_FLOAT(0, 0, 20),
		GUI_PARAM_SET_FLOAT(0, 0, 20),
	};
	GUI_PARAM_SET_FLOAT Paramset__MaxError_0cross_jerk[NUM_FREQ_ZONES] = {
		GUI_PARAM_SET_FLOAT(3, 0, 20),
		GUI_PARAM_SET_FLOAT(2, 0, 20),
		GUI_PARAM_SET_FLOAT(2, 0, 20),
	};
	
	
	/********************
	********************/
	ofxGuiGroup Group_LPF;
		ofxFloatSlider LPF_dt__FFTGain;
		
		ofxFloatSlider LPF_dt__Time_Raw;
		ofxFloatSlider LPF_dt__Time_Vel;
		ofxFloatSlider LPF_dt__Time_Acc;
		ofxFloatSlider LPF_dt__Time_Jerk;
		
	ofxGuiGroup Group_Zone;
		ofxFloatSlider Zone_FreqFrom[NUM_FREQ_ZONES];
		ofxFloatSlider Zone_FreqWidth[NUM_FREQ_ZONES];
		ofxFloatSlider CancelLineIn_Gain[NUM_FREQ_ZONES];
		
	ofxGuiGroup Group_GraphScale_FFT;
		ofxFloatSlider GraphScale_Zoom_FreqFrom;
	
		ofxFloatSlider GraphScale_FFTGain_Monitor;
		ofxFloatSlider GraphScale_FFTGain_Monitor_H;
		
		ofxFloatSlider GraphScale_FFTGain_Diff;
		ofxFloatSlider GraphScale_FFTGain_Diff_H;

	ofxGuiGroup Group_GraphScale_Time;
		ofxFloatSlider GraphScale_Time_Raw[NUM_FREQ_ZONES];
		ofxFloatSlider GraphScale_Time_Vel[NUM_FREQ_ZONES];
		ofxFloatSlider GraphScale_Time_Acc[NUM_FREQ_ZONES];
		ofxFloatSlider GraphScale_Time_Jerk[NUM_FREQ_ZONES];
		
	ofxGuiGroup Group_thresh;
		ofxFloatSlider thresh_raw_L[NUM_FREQ_ZONES];
		ofxFloatSlider thresh_vel_H[NUM_FREQ_ZONES];
		ofxFloatSlider thresh_acc_H[NUM_FREQ_ZONES];
		
	ofxGuiGroup Group_Hys_0cross;
		ofxFloatSlider Hys_0cross_vel[NUM_FREQ_ZONES];
		ofxFloatSlider Hys_0cross_acc[NUM_FREQ_ZONES];
		ofxFloatSlider Hys_0cross_jerk[NUM_FREQ_ZONES];
		
	ofxGuiGroup Group_MaxError_ZeroCross;
		ofxFloatSlider MaxError_0cross_vel[NUM_FREQ_ZONES];
		ofxFloatSlider MaxError_0cross_acc[NUM_FREQ_ZONES];
		ofxFloatSlider MaxError_0cross_jerk[NUM_FREQ_ZONES];
	
	/****************************************
	****************************************/
	ofxPanel gui;
};

/************************************************************
************************************************************/
double LPF(double LastVal, double CurrentVal, double Alpha_dt, double dt);
double LPF(double LastVal, double CurrentVal, double Alpha);
double sj_max(double a, double b);

/************************************************************
************************************************************/
extern GUI_GLOBAL* Gui_Global;

extern FILE* fp_Log;
extern FILE* fp_Log_main;
extern FILE* fp_Log_fft;

extern int GPIO_0;
extern int GPIO_1;

extern bool b_PauseGraph;

extern OSC_TARGET Osc[];

/************************************************************
************************************************************/

