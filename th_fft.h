/************************************************************
************************************************************/
#pragma once

/************************************************************
************************************************************/
#include "ofMain.h"

#include "sj_common.h"

/************************************************************
************************************************************/

/**************************************************
**************************************************/
class ZERO_CROSS : private Noncopyable{
private:
	enum STATE{
		STATE__L,
		STATE__H,
		
		NUM_STATES,
	};
	
	STATE State = STATE__L;
	int ZeroCross = 0;
	
public:
	void Reset(double val){
		if(0 <= val)	State = STATE__H;
		else			State = STATE__L;
		
		ZeroCross = 0;
	}
	
	void update(double val, double thresh){
		if(State == STATE__L){
			if(thresh < val){
				State = STATE__H;
				ZeroCross++;
			}
		}else{
			if(val < -thresh){
				State = STATE__L;
				ZeroCross++;
			}
		}
	}
	
	int get_ZeroCross()	{ return ZeroCross; }
};

/**************************************************
**************************************************/
class STATE_CHART : private Noncopyable{
private:
	enum STATE{
		STATE__WINDOW_L,
		STATE__WINDOW_M,
		STATE__WINDOW_H,
		
		NUM_STATES,
	};
	
	STATE State = STATE__WINDOW_L;
	bool b_used = false;
	
	ZERO_CROSS ZeroCross_vel;
	ZERO_CROSS ZeroCross_acc;
	ZERO_CROSS ZeroCross_jerk;
	ZERO_CROSS ZeroCross_vel__NoHys; // for debug.
	ZERO_CROSS ZeroCross_acc__NoHys; // for debug.
	ZERO_CROSS ZeroCross_jerk__NoHys; // for debug.
	
	int ZeroCrossVel_ErrorWithin;
	int ZeroCrossAcc_ErrorWithin;
	int ZeroCrossJerk_ErrorWithin;
	
	double Raw_max;
	double thresh_Raw_dynamic;
	// const double thresh_Raw_dynamic_den = 7.0;
	const double thresh_Raw_dynamic_den = 7.5;
	
	double vel_max;
	int t_from_H;
	bool b_checkJerkDuration;
	int JerkDuration = 0; // for Log.
	
	int ScoredTime = -1;
	
	string Name = "NotSet"; // for Log.
	
	
	void update_H(int now_ms, double* Time_Raw, double* Time_Vel, double* Time_Acc, double* Time_Jerk, double thresh_Raw, double thresh_Vel, double Hys0cross_Vel, double Hys0cross_Acc, double Hys0cross_Jerk){
		if(Raw_max < Time_Raw[FBO_TIME_WIDTH - 1]){
			Raw_max = Time_Raw[FBO_TIME_WIDTH - 1];
			thresh_Raw_dynamic = Raw_max/thresh_Raw_dynamic_den;
		}
		
		if(Time_Raw[FBO_TIME_WIDTH - 1] < max(thresh_Raw_dynamic, thresh_Raw))	Transition_HtoM(now_ms, thresh_Vel);
		else																	update_H_stay(now_ms, Time_Vel, Time_Acc, Time_Jerk, Hys0cross_Vel, Hys0cross_Acc, Hys0cross_Jerk);
	}
	
	void update_H_stay(int now_ms, double* Time_Vel, double* Time_Acc, double* Time_Jerk, double Hys0cross_Vel, double Hys0cross_Acc, double Hys0cross_Jerk){
		ZeroCross_vel.update(Time_Vel[FBO_TIME_WIDTH - 1], Hys0cross_Vel);
		ZeroCross_acc.update(Time_Acc[FBO_TIME_WIDTH - 1], Hys0cross_Acc);
		ZeroCross_jerk.update(Time_Jerk[FBO_TIME_WIDTH - 1], Hys0cross_Jerk);
		ZeroCross_vel__NoHys.update(Time_Vel[FBO_TIME_WIDTH - 1], 0);
		ZeroCross_acc__NoHys.update(Time_Acc[FBO_TIME_WIDTH - 1], 0);
		ZeroCross_jerk__NoHys.update(Time_Jerk[FBO_TIME_WIDTH - 1], 0);
		
		if(vel_max < Time_Vel[FBO_TIME_WIDTH - 1])	vel_max = Time_Vel[FBO_TIME_WIDTH - 1];
		
		if(b_checkJerkDuration && Time_Jerk[FBO_TIME_WIDTH - 1] < 0){
			JerkDuration = now_ms - t_from_H;
			b_checkJerkDuration = false;
		}
	}
	
	void Transition_HtoM(int now_ms, double thresh_Vel){
		State = STATE__WINDOW_M;
		ScoredTime = now_ms;
		b_used = false;
		
		printResult(thresh_Vel); // verbose
	}
	
	void printResult(double thresh_Vel){
		if(!b_PauseGraph){
			printf("----------\n");
			printf("> %s\n", Name.c_str());
			printf("> ScoredTime = %f\n", (double)ScoredTime/1000);
			
			int ZeroCrossWhenNoNoise = 1;
			printf("> ZeroCross_vel = %d (NoHys = %d)\n", ZeroCross_vel.get_ZeroCross(), ZeroCross_vel__NoHys.get_ZeroCross());
			printf("\t(Error, within) = (%2d, %2d) = %d\n", max(ZeroCross_vel.get_ZeroCross() - ZeroCrossWhenNoNoise, 0), ZeroCrossVel_ErrorWithin, ZeroCrossVel_ErrorWithin - max(ZeroCross_vel.get_ZeroCross() - ZeroCrossWhenNoNoise, 0));
			
			ZeroCrossWhenNoNoise = 2;
			printf("> ZeroCross_acc = %d (NoHys = %d)\n", ZeroCross_acc.get_ZeroCross(), ZeroCross_acc__NoHys.get_ZeroCross());
			printf("\t(Error, within) = (%2d, %2d) = %d\n", max(ZeroCross_acc.get_ZeroCross() - ZeroCrossWhenNoNoise, 0), ZeroCrossAcc_ErrorWithin, ZeroCrossAcc_ErrorWithin - max(ZeroCross_acc.get_ZeroCross() - ZeroCrossWhenNoNoise, 0));
			
			ZeroCrossWhenNoNoise = 3;
			printf("> ZeroCross_jer = %d (NoHys = %d)\n", ZeroCross_jerk.get_ZeroCross(), ZeroCross_jerk__NoHys.get_ZeroCross());
			printf("\t(Error, within) = (%2d, %2d) = %d\n", max(ZeroCross_jerk.get_ZeroCross() - ZeroCrossWhenNoNoise, 0), ZeroCrossJerk_ErrorWithin, ZeroCrossJerk_ErrorWithin - max(ZeroCross_jerk.get_ZeroCross() - ZeroCrossWhenNoNoise, 0));
			
			printf("> vel_max = %e(/ %e)\n", vel_max, thresh_Vel);
			printf("> JerkDuration = %d\n", JerkDuration);
			printf("\n");
			fflush(stdout);
		}
	}
	
	void update_L(int now_ms, double* Time_Raw, double* Time_Vel, double* Time_Acc, double* Time_Jerk, double thresh_Acc){
		if( (Time_Acc[FBO_TIME_WIDTH - 2] < thresh_Acc) && (thresh_Acc <= Time_Acc[FBO_TIME_WIDTH - 1]) ){
			State = STATE__WINDOW_H;
			ZeroCross_vel.Reset(Time_Vel[FBO_TIME_WIDTH - 1]);
			ZeroCross_acc.Reset(Time_Acc[FBO_TIME_WIDTH - 1]);
			ZeroCross_jerk.Reset(Time_Jerk[FBO_TIME_WIDTH - 1]);
			ZeroCross_vel__NoHys.Reset(Time_Vel[FBO_TIME_WIDTH - 1]);
			ZeroCross_acc__NoHys.Reset(Time_Acc[FBO_TIME_WIDTH - 1]);
			ZeroCross_jerk__NoHys.Reset(Time_Jerk[FBO_TIME_WIDTH - 1]);
			
			vel_max = Time_Vel[FBO_TIME_WIDTH - 1];
			t_from_H = now_ms;
			b_checkJerkDuration = true;
			JerkDuration = -1;
			
			Raw_max = Time_Raw[FBO_TIME_WIDTH - 1];
			thresh_Raw_dynamic = Raw_max/thresh_Raw_dynamic_den;
		}
	}
	
	void update_M(double* Time_Raw, double thresh_Raw){
		if(Time_Raw[FBO_TIME_WIDTH - 1] < thresh_Raw){
			State = STATE__WINDOW_L;
		}
	}
	
	bool IsState_TimeToChallange(){
		if( (State == STATE__WINDOW_L) || (State == STATE__WINDOW_M) )	return true;
		else															return false;
	}
	
public:
	STATE_CHART()
	: ZeroCrossVel_ErrorWithin(0), ZeroCrossAcc_ErrorWithin(0), ZeroCrossJerk_ErrorWithin(0)
	{
	}
	
	STATE_CHART(int _ZeroCrossVel_ErrorWithin, int _ZeroCrossAcc_ErrorWithin, int _ZeroCrossJerk_ErrorWithin)
	: ZeroCrossVel_ErrorWithin(_ZeroCrossVel_ErrorWithin), ZeroCrossAcc_ErrorWithin(_ZeroCrossAcc_ErrorWithin), ZeroCrossJerk_ErrorWithin(_ZeroCrossJerk_ErrorWithin)
	{
	}
	
	void setup(string _Name){
		Name = _Name;
	}
	
	void set_MaxError_ZeroCross(int _ZeroCrossVel_ErrorWithin, int _ZeroCrossAcc_ErrorWithin, int _ZeroCrossJerk_ErrorWithin){
		ZeroCrossVel_ErrorWithin = _ZeroCrossVel_ErrorWithin;
		ZeroCrossAcc_ErrorWithin = _ZeroCrossAcc_ErrorWithin;
		ZeroCrossJerk_ErrorWithin = _ZeroCrossJerk_ErrorWithin;
	}
	
	void update(int now_ms, double* Time_Raw, double* Time_Vel, double* Time_Acc, double* Time_Jerk, double thresh_Raw, double thresh_Vel, double thresh_Acc, double Hys0cross_Vel, double Hys0cross_Acc, double Hys0cross_Jerk){
		if(State == STATE__WINDOW_L)		update_L(now_ms, Time_Raw, Time_Vel, Time_Acc, Time_Jerk, thresh_Acc);
		else if(State == STATE__WINDOW_H)	update_H(now_ms, Time_Raw, Time_Vel, Time_Acc, Time_Jerk, thresh_Raw, thresh_Vel, Hys0cross_Vel, Hys0cross_Acc, Hys0cross_Jerk);
		else if(State == STATE__WINDOW_M)	update_M(Time_Raw, thresh_Raw);
	}
	
	bool IsJustScored(int now){
		if(ScoredTime == now)	return true;
		else					return false;
	}
	
	bool IsTimeToChallenge(int now){
		if( (IsState_TimeToChallange()) && (now - ScoredTime <= 1000) && (!b_used) )	return true;
		else																			return false;
	}
	
	bool Is_VelMax_Clear(double thresh_Vel){
		if(thresh_Vel < vel_max)	return true;
		else						return false;
	}
	
	bool Is_Got_JerkDutaion(){
		if(!b_checkJerkDuration)	return true;
		else						return false;
	}
	
	bool Is_JerkDurationClear(){
		if( (JerkDuration != -1) && (JerkDuration < 50) )	return true;
		else												return false;
	}
	
	bool Is_ZeroCrossVel_Clear(){
		int ZeroCrossWhenNoNoise = 1;
		if( 0 <  max( max(ZeroCross_vel.get_ZeroCross() - ZeroCrossWhenNoNoise, 0) - ZeroCrossVel_ErrorWithin, 0 ) )	return false;
		else																											return true;
	}
	
	bool Is_ZeroCrossAcc_Clear(){
		int ZeroCrossWhenNoNoise = 2;
		if( 0 <  max( max(ZeroCross_acc.get_ZeroCross() - ZeroCrossWhenNoNoise, 0) - ZeroCrossAcc_ErrorWithin, 0 ) )	return false;
		else																											return true;
	}
	
	bool Is_ZeroCrossJerk_Clear(){
		int ZeroCrossWhenNoNoise = 3;
		if( 0 <  max( max(ZeroCross_jerk.get_ZeroCross() - ZeroCrossWhenNoNoise, 0) - ZeroCrossJerk_ErrorWithin, 0 ) )	return false;
		else																											return true;
	}
	
	static int get_NumStates() { return NUM_STATES; }
	
	int get_State()	{ return (int)State; }
	
	void ForcedReset(int now_ms){
		if(State == STATE__WINDOW_H){
			State = STATE__WINDOW_L;
			ScoredTime = now_ms;
		}
	}
	
	void SetUsed() { b_used = true; }
};

/**************************************************
**************************************************/
class THREAD_FFT : public ofThread, private Noncopyable{
private:
	/****************************************
	****************************************/
	enum STATE{
		STATE__WINDOW_L,
		STATE__WINDOW_H,
		
		NUM_STATES,
	};
	
	enum{
		NUM_SAMPLE_ADJUSTSCALE = 10,
	};
	
	/****************************************
	****************************************/
	
	/********************
	********************/
	STATE_CHART StateChart[NUM_FREQ_ZONES];
	
	/********************
	********************/
	double Gain_Monitor[AUDIO_BUF_SIZE];
	double Gain_LineIn[AUDIO_BUF_SIZE];
	double Gain_Diff[AUDIO_BUF_SIZE];
	
	double Time_Raw[NUM_FREQ_ZONES][FBO_TIME_WIDTH];
	double Time_Vel[NUM_FREQ_ZONES][FBO_TIME_WIDTH];
	double Time_Acc[NUM_FREQ_ZONES][FBO_TIME_WIDTH];
	double Time_Jerk[NUM_FREQ_ZONES][FBO_TIME_WIDTH];
	double Time_State[NUM_FREQ_ZONES][FBO_TIME_WIDTH];
	
	bool Time_DetectClap[FBO_TIME_WIDTH];
	
	/* */
	int Array_AdjustScale_StampId[FBO_TIME_WIDTH];
	bool b_AdjustScale;
	int AdjustScale_StampId;
	const int AdjustScale_IdInterval;
	int AdjustScale_NumSampled;
	
	double AdjustScale_RawMax_sample[NUM_FREQ_ZONES][NUM_SAMPLE_ADJUSTSCALE];
	double AdjustScale_VelMax_sample[NUM_FREQ_ZONES][NUM_SAMPLE_ADJUSTSCALE];
	double AdjustScale_AccMax_sample[NUM_FREQ_ZONES][NUM_SAMPLE_ADJUSTSCALE];
	double AdjustScale_JerkMax_sample[NUM_FREQ_ZONES][NUM_SAMPLE_ADJUSTSCALE];
	
	
	/* */
	const int N;
	
	vector<float> fft_window;
	vector<double> sintbl;
	vector<int> bitrev;
	
	int LastInt;
	
	/****************************************
	function
	****************************************/
	/********************
	singleton
	********************/
	THREAD_FFT();
	~THREAD_FFT();
	THREAD_FFT(const THREAD_FFT&); // Copy constructor. no define.
	THREAD_FFT& operator=(const THREAD_FFT&); // コピー代入演算子. no define.
	
	/********************
	********************/
	template<class T>
	void update_Array(T* Array, int ArraySize, T NewVal)
	{
		for(int i = 0; i < ArraySize - 1; i++){
			Array[i] = Array[i + 1];
		}
		
		Array[ArraySize - 1] = NewVal;
	}
	

	
	/********************
	********************/
	void threadedFunction();
	
	int fft(double x[], double y[], int IsReverse = false);
	void make_bitrev(void);
	void make_sintbl(void);
	void AudioSample_fft_LPF_saveToArray(const vector<float> &AudioSample, double Gain[], float dt);
	
	void update__GainDiff();
	
	void update__Time_Raw(double dt);
	int cal_Fix_AccessRange(int& from, int width, int _max);
	void update_TimeArray(double* Array, int ArraySize, double NewVal, double LPF_dt, double dt);
	
	void update__Time_Delta(double (*ArrayFrom)[FBO_TIME_WIDTH], double (*ArrayTo)[FBO_TIME_WIDTH], double dt_LPF, double dt);
	
	void Reset__Time_AdjustScale_id();
	void update__AdjustScale();
	double get_max_ofArray(double* array, int from, int to);
	double get_AbsMax_ofArray(double* array, int from, int to);
	void SetScale(int zone, double raw, double vel, double acc, double jerk);
	void SaveScaleData_toFile();
	
	static int double_sort( const void * a , const void * b );
	static double get_ave_of_array(double* array, int from, int to);
	static double get_center_of_array(double* array, int from, int to);
	static double get_Lower_of_array(double* array, int from, int to);
	static double get_ratio_of_array(double* array, int from, int to, double ratio);
	
	void StateChart_AllZone(int now);
	void StateChart_Total(int now);

	
public:
	/****************************************
	****************************************/
	/********************
	********************/
	static THREAD_FFT* getInstance(){
		static THREAD_FFT inst;
		return &inst;
	}
	
	void exit();
	void setup();
	void update();
	
	void update__Gain(const vector<float> &AudioSample_L, const vector<float> &AudioSample_R);
	double getVal__FreqArray_x_GraphScale(int ArrayId, int id, float GraphScale, float ScreenHeight, bool b_Clamp);
	double getVal__TimeArray_x_GraphScale(int ZoneId, int ArrayId, int id, float GraphScale, float ScreenHeight, bool b_Clamp);
	bool getVal__TimeArray_DetectClap(int id);
	double getVal__AdjustScaleId_x_ScreenHeight(int id, float ScreenHeight);
	
	void Start_AdjustScale();
	bool Is_AdjustingScale();
};



