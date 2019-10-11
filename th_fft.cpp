/************************************************************
************************************************************/
#include "th_fft.h"
#include "stdlib.h"

#include <new> // placement new

/************************************************************
************************************************************/

/******************************
******************************/
THREAD_FFT::THREAD_FFT()
: N(AUDIO_BUF_SIZE)
, LastInt(0)
, b_AdjustScale(false)
, AdjustScale_StampId(0)
// , AdjustScale_IdInterval(430) // 5sec : 512 sample
// , AdjustScale_IdInterval(215) // 5sec : 1024 sample
, AdjustScale_IdInterval(172) // 4sec : 1024 sample
, AdjustScale_NumSampled(0)
{
	/********************
	********************/
	/* 窓関数 */
	fft_window.resize(N);
	for(int i = 0; i < N; i++)	fft_window[i] = 0.5 - 0.5 * cos(2 * PI * i / N);
	
	sintbl.resize(N + N/4);
	bitrev.resize(N);
	
	make_bitrev();
	make_sintbl();
	
	/********************
	********************/
	setup();
}

/******************************
******************************/
THREAD_FFT::~THREAD_FFT()
{
}

/******************************
******************************/
void THREAD_FFT::threadedFunction()
{
	while(isThreadRunning()) {
		lock();
		
		unlock();
		
		
		sleep(1);
	}
}

/******************************
******************************/
void THREAD_FFT::exit()
{
}

/******************************
******************************/
void THREAD_FFT::setup()
{
	this->lock();
		for(int i = 0; i < AUDIO_BUF_SIZE; i++){
			Gain_Monitor[i] = 0;
			Gain_LineIn[i] = 0;
			Gain_Diff[i] = 0;
		}
		
		for(int i = 0; i < NUM_FREQ_ZONES; i++){
			for(int j = 0; j < FBO_TIME_WIDTH; j++){
				Time_Raw[i][j] = 0;
				Time_Vel[i][j] = 0;
				Time_Acc[i][j] = 0;
				Time_Jerk[i][j] = 0;
				Time_State[i][j] = 0;
			}
		}
		
		for(int i = 0; i < FBO_TIME_WIDTH; i++){
			Time_DetectClap[i] = false;
		}
		
		Reset__Time_AdjustScale_id();
		
		for(int zone = 0; zone < NUM_FREQ_ZONES; zone++){
			char buf[BUF_SIZE_S];
			sprintf(buf, "StateChart_%d", zone);
			StateChart[zone].setup(buf);
		}
	this->unlock();
}

/******************************
******************************/
void THREAD_FFT::Reset__Time_AdjustScale_id()
{
	for(int i = 0; i < FBO_TIME_WIDTH; i++){
		Array_AdjustScale_StampId[i] = -1;
	}
}

/******************************
******************************/
double THREAD_FFT::getVal__FreqArray_x_GraphScale(int ArrayId, int id, float GraphScale, float ScreenHeight, bool b_Clamp)
{
	if((id < 0) || (AUDIO_BUF_SIZE/2 <= id)) return 0;
	
	double* array;
	if(ArrayId == 0)		array = Gain_Monitor;
	else if(ArrayId == 1)	array = Gain_LineIn;
	else if(ArrayId == 2)	array = Gain_Diff;
	else					return 0;
	
	
	
	double ret = 0;
	
	this->lock();
		ret = ofMap(array[id], 0, GraphScale, 0, ScreenHeight, b_Clamp); // b_Clamp = false : マイナス側もok.
	this->unlock();
	
	return ret;
}

/******************************
******************************/
double THREAD_FFT::getVal__TimeArray_x_GraphScale(int ZoneId, int ArrayId, int id, float GraphScale, float ScreenHeight, bool b_Clamp)
{
	if( (ZoneId < 0) || (NUM_FREQ_ZONES <= ZoneId) ) { ERROR_MSG(); std::exit(1); }
	if((id < 0) || (FBO_TIME_WIDTH <= id)) return 0;
	
	double* array;
	if(ArrayId == 0)		array = Time_Raw[ZoneId];
	else if(ArrayId == 1)	array = Time_Vel[ZoneId];
	else if(ArrayId == 2)	array = Time_Acc[ZoneId];
	else if(ArrayId == 3)	array = Time_Jerk[ZoneId];
	else if(ArrayId == 4)	array = Time_State[ZoneId];
	else					return 0;
	
	
	
	double ret = 0;
	
	this->lock();
		ret = ofMap(array[id], 0, GraphScale, 0, ScreenHeight, b_Clamp);
	this->unlock();
	
	return ret;
}

/******************************
******************************/
bool THREAD_FFT::getVal__TimeArray_DetectClap(int id)
{
	if((id < 0) || (FBO_TIME_WIDTH <= id)) return false;
	
	
	bool ret = false;
	
	this->lock();
		ret = Time_DetectClap[id];
	this->unlock();
	
	return ret;
}

/******************************
******************************/
double THREAD_FFT::getVal__AdjustScaleId_x_ScreenHeight(int id, float ScreenHeight)
{
	if((id < 0) || (FBO_TIME_WIDTH <= id)) return 0;
	
	
	double ret = 0;
	
	this->lock();
		if(Array_AdjustScale_StampId[id] == 0){
			ret = ScreenHeight/2;
		}else if(Array_AdjustScale_StampId[id] % AdjustScale_IdInterval == 0){
			ret = ScreenHeight;
		}else{
			ret = -1;
		}
	this->unlock();
	
	return ret;
}

/******************************
******************************/
void THREAD_FFT::Start_AdjustScale()
{
	this->lock();
		Reset__Time_AdjustScale_id();
		
		b_AdjustScale = true;
		AdjustScale_StampId = 0;
		AdjustScale_NumSampled = 0;
	this->unlock();
}

/******************************
******************************/
bool THREAD_FFT::Is_AdjustingScale()
{
	bool ret;
	
	this->lock();
		ret = b_AdjustScale;
	this->unlock();
	
	return ret;
}

/******************************
description
	昇順
******************************/
int THREAD_FFT::double_sort( const void * a , const void * b )
{
	if(*(double*)a < *(double*)b){
		return -1;
	}else if(*(double*)a == *(double*)b){
		return 0;
	}else{
		return 1;
	}
}

/******************************
******************************/
void THREAD_FFT::update()
{
	this->lock();
	
	this->unlock();
}

/******************************
******************************/
void THREAD_FFT::update__Gain(const vector<float> &AudioSample_L, const vector<float> &AudioSample_R)
{
	/********************
	********************/
	const int duration = 10e+3;
	
	int now = (int)ofGetElapsedTimeMillis();
	if(fp_Log_fft != NULL) { if(now < duration) fprintf(fp_Log_fft, "%d,", now); }
	
	/********************
	********************/
	this->lock();
		/* */
		AudioSample_fft_LPF_saveToArray(AudioSample_L, Gain_Monitor, double(now - LastInt)/1000);
		AudioSample_fft_LPF_saveToArray(AudioSample_R, Gain_LineIn, double(now - LastInt)/1000);
		
		/* */
		update__GainDiff();
		
		/* */
		update__Time_Raw(double(now - LastInt)/1000);
		update__Time_Delta(Time_Raw, Time_Vel, Gui_Global->LPF_dt__Time_Vel, double(now - LastInt)/1000);
		update__Time_Delta(Time_Vel, Time_Acc, Gui_Global->LPF_dt__Time_Acc, double(now - LastInt)/1000);
		update__Time_Delta(Time_Acc, Time_Jerk, Gui_Global->LPF_dt__Time_Jerk, double(now - LastInt)/1000);
		
		/* */
		if(b_AdjustScale)	update__AdjustScale();
		
		/* */
		StateChart_AllZone(now);
		StateChart_Total(now);
		
	this->unlock();
		
	/********************
	********************/
	LastInt = now;
	
	if(fp_Log_fft != NULL){
		if(now < duration)	{ fprintf(fp_Log_fft, "%d\n", (int)ofGetElapsedTimeMillis()); }
		else				{ fclose(fp_Log_fft); fp_Log_fft = NULL; }
	}
}

/******************************
******************************/
void THREAD_FFT::update__AdjustScale()
{
	update_Array(Array_AdjustScale_StampId, FBO_TIME_WIDTH, AdjustScale_StampId);
	
	if( (AdjustScale_StampId % AdjustScale_IdInterval == 0) && (AdjustScale_StampId/AdjustScale_IdInterval == 1) ){
		for(int zone = 0; zone < NUM_FREQ_ZONES; zone++){
			const double Boost = 2.5;
			Gui_Global->Hys_0cross_vel[zone] = get_AbsMax_ofArray(Time_Vel[zone], FBO_TIME_WIDTH - AdjustScale_IdInterval, FBO_TIME_WIDTH - 1) * Boost;
			Gui_Global->Hys_0cross_acc[zone] = get_AbsMax_ofArray(Time_Acc[zone], FBO_TIME_WIDTH - AdjustScale_IdInterval, FBO_TIME_WIDTH - 1) * Boost;
			Gui_Global->Hys_0cross_jerk[zone] = get_AbsMax_ofArray(Time_Jerk[zone], FBO_TIME_WIDTH - AdjustScale_IdInterval, FBO_TIME_WIDTH - 1) * Boost;
		}
		
	}else if( (AdjustScale_StampId % AdjustScale_IdInterval == 0) && (2 <= AdjustScale_StampId/AdjustScale_IdInterval) ){
		for(int zone = 0; zone < NUM_FREQ_ZONES; zone++){
			AdjustScale_RawMax_sample[zone][AdjustScale_NumSampled] = get_max_ofArray(Time_Raw[zone], FBO_TIME_WIDTH - AdjustScale_IdInterval, FBO_TIME_WIDTH - 1);
			AdjustScale_VelMax_sample[zone][AdjustScale_NumSampled] = get_max_ofArray(Time_Vel[zone], FBO_TIME_WIDTH - AdjustScale_IdInterval, FBO_TIME_WIDTH - 1);
			AdjustScale_AccMax_sample[zone][AdjustScale_NumSampled] = get_max_ofArray(Time_Acc[zone], FBO_TIME_WIDTH - AdjustScale_IdInterval, FBO_TIME_WIDTH - 1);
			AdjustScale_JerkMax_sample[zone][AdjustScale_NumSampled] = get_max_ofArray(Time_Jerk[zone], FBO_TIME_WIDTH - AdjustScale_IdInterval, FBO_TIME_WIDTH - 1);
		}
		
		AdjustScale_NumSampled++;
		
		
		double (*pFunc)(double*, int, int);
		// pFunc = get_ave_of_array;
		// pFunc = get_center_of_array;
		pFunc = get_Lower_of_array;
		
		for(int zone = 0; zone < NUM_FREQ_ZONES; zone++){
			double ave_Raw = pFunc(AdjustScale_RawMax_sample[zone], 0, AdjustScale_NumSampled - 1);
			double ave_Vel = pFunc(AdjustScale_VelMax_sample[zone], 0, AdjustScale_NumSampled - 1);
			double ave_Acc = pFunc(AdjustScale_AccMax_sample[zone], 0, AdjustScale_NumSampled - 1);
			double ave_Jerk = pFunc(AdjustScale_JerkMax_sample[zone], 0, AdjustScale_NumSampled - 1);
			
			SetScale(zone, ave_Raw, ave_Vel, ave_Acc, ave_Jerk);
			
			if(NUM_SAMPLE_ADJUSTSCALE <= AdjustScale_NumSampled){
				Reset__Time_AdjustScale_id();
				b_AdjustScale = false;
				SaveScaleData_toFile();
			}
		}
	}
	
	AdjustScale_StampId++;
}

/******************************
******************************/
void THREAD_FFT::SaveScaleData_toFile()
{
	/********************
	********************/
	FILE* fp = fopen("../../../data/GUI_Scale.txt", "w");
	if(fp == NULL) { printf("Unable to open GUI_Scale.txt\n"); fflush(stdout); return;}
	
	char buf[BUF_SIZE_S];
	
	/********************
	********************/
	sprintf(buf, "<GraphScale_FFTGain_Monitor> %e %e %e\n", (float)Gui_Global->GraphScale_FFTGain_Monitor, 0.0, (float)Gui_Global->GraphScale_FFTGain_Monitor * 2);
	fprintf(fp, "%s", buf);
	sprintf(buf, "<GraphScale_FFTGain_Monitor_H> %e %e %e\n", (float)Gui_Global->GraphScale_FFTGain_Monitor_H, 0.0, (float)Gui_Global->GraphScale_FFTGain_Monitor_H * 2);
	fprintf(fp, "%s", buf);
	
	sprintf(buf, "<GraphScale_FFTGain_Diff> %e %e %e\n", (float)Gui_Global->GraphScale_FFTGain_Diff, 0.0, (float)Gui_Global->GraphScale_FFTGain_Diff * 2);
	fprintf(fp, "%s", buf);
	sprintf(buf, "<GraphScale_FFTGain_Diff_H> %e %e %e\n", (float)Gui_Global->GraphScale_FFTGain_Diff_H, 0.0, (float)Gui_Global->GraphScale_FFTGain_Diff_H * 2);
	fprintf(fp, "%s", buf);
	
	/********************
	********************/
	for(int zone = 0; zone < NUM_FREQ_ZONES; zone++){
		/* */
		sprintf(buf, "<GraphScale_Time_Raw[%d]> %e %e %e\n", zone, (float)Gui_Global->GraphScale_Time_Raw[zone], 0.0, (float)Gui_Global->GraphScale_Time_Raw[zone] * 2);
		fprintf(fp, "%s", buf);

		sprintf(buf, "<GraphScale_Time_Vel[%d]> %e %e %f\n", zone, (float)Gui_Global->GraphScale_Time_Vel[zone], 0.0, (float)Gui_Global->GraphScale_Time_Vel[zone] * 2);
		fprintf(fp, "%s", buf);

		sprintf(buf, "<GraphScale_Time_Acc[%d]> %e %e %e\n", zone, (float)Gui_Global->GraphScale_Time_Acc[zone], 0.0, (float)Gui_Global->GraphScale_Time_Acc[zone] * 2);
		fprintf(fp, "%s", buf);

		sprintf(buf, "<GraphScale_Time_Jerk[%d]> %e %e %e\n", zone, (float)Gui_Global->GraphScale_Time_Jerk[zone], 0.0, (float)Gui_Global->GraphScale_Time_Jerk[zone] * 2);
		fprintf(fp, "%s", buf);
		
		/* */
		sprintf(buf, "<thresh_raw_L[%d]> %e %e %e\n", zone, (float)Gui_Global->thresh_raw_L[zone], 0.0, (float)Gui_Global->thresh_raw_L[zone] * 2);
		fprintf(fp, "%s", buf);
		sprintf(buf, "<thresh_vel_H[%d]> %e %e %e\n", zone, (float)Gui_Global->thresh_vel_H[zone], 0.0, (float)Gui_Global->thresh_vel_H[zone] * 2);
		fprintf(fp, "%s", buf);
		sprintf(buf, "<thresh_acc_H[%d]> %e %e %e\n", zone, (float)Gui_Global->thresh_acc_H[zone], 0.0, (float)Gui_Global->thresh_acc_H[zone] * 2);
		fprintf(fp, "%s", buf);
		
		/* */
		sprintf(buf, "<Hys_0cross_vel[%d]> %e %e %e\n", zone, (float)Gui_Global->Hys_0cross_vel[zone], 0.0, (float)Gui_Global->Hys_0cross_vel[zone] * 2);
		fprintf(fp, "%s", buf);
		sprintf(buf, "<Hys_0cross_acc[%d]> %e %e %e\n", zone, (float)Gui_Global->Hys_0cross_acc[zone], 0.0, (float)Gui_Global->Hys_0cross_acc[zone] * 2);
		fprintf(fp, "%s", buf);
		sprintf(buf, "<Hys_0cross_jerk[%d]> %e %e %e\n", zone, (float)Gui_Global->Hys_0cross_jerk[zone], 0.0, (float)Gui_Global->Hys_0cross_jerk[zone] * 2);
		fprintf(fp, "%s", buf);
	}
	
	/********************
	********************/
	fclose(fp);
}

/******************************
******************************/
void THREAD_FFT::SetScale(int zone, double raw, double vel, double acc, double jerk)
{
	Gui_Global->GraphScale_Time_Raw[zone] = raw * 2;
	Gui_Global->GraphScale_Time_Vel[zone] = vel * 2;
	Gui_Global->GraphScale_Time_Acc[zone] = acc * 2;
	Gui_Global->GraphScale_Time_Jerk[zone] = jerk * 2;
	
	if(zone == 0){
		Gui_Global->thresh_raw_L[zone] = raw / 5;
		Gui_Global->thresh_vel_H[zone] = vel / 5;
		Gui_Global->thresh_acc_H[zone] = acc / 5;
	}else{
		Gui_Global->thresh_raw_L[zone] = raw / 3;
		Gui_Global->thresh_vel_H[zone] = vel / 3;
		Gui_Global->thresh_acc_H[zone] = acc / 5; // window は開きやすくしておく.
	}
	
	if(zone == 0){
		Gui_Global->GraphScale_FFTGain_Monitor = raw * 2;
		Gui_Global->GraphScale_FFTGain_Diff = raw * 2;
	}else if(zone == 2){
		Gui_Global->GraphScale_FFTGain_Monitor_H = raw * 2;
		Gui_Global->GraphScale_FFTGain_Diff_H = raw * 2;
	}
}

/******************************
******************************/
double THREAD_FFT::get_max_ofArray(double* array, int from, int to)
{
	double _max = array[from];
	
	for(int i = from + 1; i <= to; i++){
		if(_max < array[i]) _max = array[i];
	}
	
	return _max;
}

/******************************
******************************/
double THREAD_FFT::get_AbsMax_ofArray(double* array, int from, int to)
{
	double _max = abs(array[from]);
	
	for(int i = from + 1; i <= to; i++){
		if(_max < abs(array[i])) _max = abs(array[i]);
	}
	
	return _max;
}

/******************************
******************************/
double THREAD_FFT::get_ave_of_array(double* array, int from, int to)
{
	double sum = 0;
	int num = 0;
	
	for(int i = from ; i <= to; i++){
		sum += array[i];
		num++;
	}
	
	if(num == 0)	return 0;
	else			return sum / num;
}

/******************************
******************************/
double THREAD_FFT::get_center_of_array(double* array, int from, int to)
{
	qsort(&array[from], to - from + 1, sizeof(array[0]), double_sort);
	
	int target = (from + to)/2;
	return array[target];
}

/******************************
******************************/
double THREAD_FFT::get_Lower_of_array(double* array, int from, int to)
{
	return get_ratio_of_array(array, from, to, 0.8);
}

/******************************
******************************/
double THREAD_FFT::get_ratio_of_array(double* array, int from, int to, double ratio)
{
	qsort(&array[from], to - from + 1, sizeof(array[0]), double_sort);
	
	int target = (int)(from + (to - from) * ratio);
	return array[target];
}

/******************************
******************************/
void THREAD_FFT::update__GainDiff()
{
	for(int i = 0; i < N/2; i++){
		float CancelLineIn_Gain = -1;
		
		for(int zone = 0; zone < NUM_FREQ_ZONES; zone++){
			if( (Gui_Global->Zone_FreqFrom[zone] <= i) && (i < Gui_Global->Zone_FreqFrom[zone] + Gui_Global->Zone_FreqWidth[zone]) ){
				CancelLineIn_Gain = Gui_Global->CancelLineIn_Gain[zone];
				break;
			}
		}
		
		if(CancelLineIn_Gain == -1){
			Gain_Diff[i] = 0;
		}else{
			Gain_Diff[i] = Gain_Monitor[i] - CancelLineIn_Gain * Gain_LineIn[i];
			if(Gain_Diff[i] < 0) Gain_Diff[i] = 0;
		}
	}
}

/******************************
******************************/
void THREAD_FFT::AudioSample_fft_LPF_saveToArray(const vector<float> &AudioSample, double Gain[], float dt)
{
	/********************
	********************/
	if( AudioSample.size() != N ) { ERROR_MSG(); std::exit(1); }
	
	/********************
	********************/
	double x[N], y[N];
	
	for(int i = 0; i < N; i++){
		x[i] = AudioSample[i] * fft_window[i];
		y[i] = 0;
	}
	
	fft(x, y);

	Gain[0] = 0;
	Gain[N/2] = 0;
	for(int i = 1; i < N/2; i++){
		double GainTemp = sqrt(x[i] * x[i] + y[i] * y[i]);
		
		Gain[i] = LPF(Gain[i], GainTemp, Gui_Global->LPF_dt__FFTGain, dt);
		Gain[N - i] = Gain[i]; // 共役(yの正負反転)だが、Gainは同じ
	}
}

/******************************
******************************/
void THREAD_FFT::update__Time_Raw(double dt)
{
	for(int zone = 0; zone < NUM_FREQ_ZONES; zone++){
		int from = Gui_Global->Zone_FreqFrom[zone];
		int width = Gui_Global->Zone_FreqWidth[zone];
		int _max = AUDIO_BUF_SIZE/2 - 1;
		int to = cal_Fix_AccessRange(from, width, _max);
		
		double _ave = get_ave_of_array(Gain_Diff, from, to);
		update_TimeArray(Time_Raw[zone], FBO_TIME_WIDTH, _ave, Gui_Global->LPF_dt__Time_Raw, dt);
	}
}

/******************************
******************************/
void THREAD_FFT::update__Time_Delta(double (*ArrayFrom)[FBO_TIME_WIDTH], double (*ArrayTo)[FBO_TIME_WIDTH], double dt_LPF, double dt)
{
	for(int zone = 0; zone < NUM_FREQ_ZONES; zone++){
		double _delta = (ArrayFrom[zone][FBO_TIME_WIDTH - 1] - ArrayFrom[zone][FBO_TIME_WIDTH - 2]) / dt;
		update_TimeArray(ArrayTo[zone], FBO_TIME_WIDTH, _delta, dt_LPF, dt);
	}
}

/******************************
******************************/
int THREAD_FFT::cal_Fix_AccessRange(int& from, int width, int _max)
{
	if(width < 0) width = 0;
	
	if(from < 0)		from = 0;
	if(_max <= from)	from = _max;
	
	int to = from + width - 1;
	if(_max <= to) to = _max;
	
	return to;
}

/******************************
******************************/
void THREAD_FFT::update_TimeArray(double* Array, int ArraySize, double NewVal, double LPF_dt, double dt)
{
	for(int i = 0; i < ArraySize - 1; i++){
		Array[i] = Array[i + 1];
	}
	
	Array[ArraySize - 1] = LPF(Array[ArraySize - 2], NewVal, LPF_dt, dt);
}

/******************************
******************************/
void THREAD_FFT::StateChart_AllZone(int now)
{
	for(int zone = 0; zone < NUM_FREQ_ZONES; zone++){
		if(b_AdjustScale){
			StateChart[zone].ForcedReset(now);
		}else{
			StateChart[zone].set_MaxError_ZeroCross((int)Gui_Global->MaxError_0cross_vel[zone], (int)Gui_Global->MaxError_0cross_acc[zone], (int)Gui_Global->MaxError_0cross_jerk[zone]);
			
			StateChart[zone].update(	now, 
										Time_Raw[zone], Time_Vel[zone], Time_Acc[zone], Time_Jerk[zone], 
										Gui_Global->thresh_raw_L[zone], Gui_Global->thresh_vel_H[zone], Gui_Global->thresh_acc_H[zone], 
										Gui_Global->Hys_0cross_vel[zone], Gui_Global->Hys_0cross_acc[zone], Gui_Global->Hys_0cross_jerk[zone]);
		}
		
		update_Array(Time_State[zone], FBO_TIME_WIDTH, (double)StateChart[zone].get_State());
	}
}

/******************************
******************************/
void THREAD_FFT::StateChart_Total(int now)
{
	/********************
	********************/
	bool b_DetectClap = true;
	
	/********************
	********************/
	if(b_PauseGraph){
		update_Array(Time_DetectClap, FBO_TIME_WIDTH, false);
		return;
	}
	
	/********************
	********************/
	int counter = 0;
	for(int zone = 0; zone < NUM_FREQ_ZONES; zone++) {
		if(StateChart[zone].IsJustScored(now)) counter++;
	}
	
	if(0 < counter){
		counter = 0;
		
		for(int zone = 0; zone < NUM_FREQ_ZONES; zone++){
			if(StateChart[zone].IsTimeToChallenge(now)) counter++;
		}
		
		if(NUM_FREQ_ZONES <= counter){
			/********************
			********************/
			printf("\n>>>>> Challenge : %f <<<<<\n", (double)now / 1000);
			for(int zone = 0; zone < NUM_FREQ_ZONES; zone++) { StateChart[zone].SetUsed(); }
			
			/* */
			int NumToClear = NUM_FREQ_ZONES;
			printf("> vel max : %d\n", NumToClear);
			counter = 0;
			for(int zone = 0; zone < NUM_FREQ_ZONES; zone++){
				if(StateChart[zone].Is_VelMax_Clear(Gui_Global->thresh_vel_H[zone]))	counter++;
			}
			if(counter < NumToClear)	{ printf("\t\tNG(%d/%d)\n", counter, NUM_FREQ_ZONES); b_DetectClap = false; }
			else						{ printf("\tOK(%d/%d)\n", counter, NUM_FREQ_ZONES); }
			
			/* */
			NumToClear = NUM_FREQ_ZONES;
			printf("> got jerk Duration : %d\n", NumToClear);
			counter = 0;
			for(int zone = 0; zone < NUM_FREQ_ZONES; zone++){
				if(StateChart[zone].Is_Got_JerkDutaion())	counter++;
			}
			if(counter < NumToClear)	{ printf("\t\tNG(%d/%d)\n", counter, NUM_FREQ_ZONES); b_DetectClap = false; }
			else						{ printf("\tOK(%d/%d)\n", counter, NUM_FREQ_ZONES); }
			
			/* */
			NumToClear = 3;
			printf("> jerk Duration : %d\n", NumToClear);
			counter = 0;
			for(int zone = 0; zone < NUM_FREQ_ZONES; zone++){
				if(StateChart[zone].Is_JerkDurationClear())	counter++;
			}
			if(counter < NumToClear)	{ printf("\t\tNG(%d/%d)\n", counter, NUM_FREQ_ZONES); b_DetectClap = false; }
			else						{ printf("\tOK(%d/%d)\n", counter, NUM_FREQ_ZONES); }
			
			/* */
			NumToClear = 3;
			printf("> ZeroCross Vel : %d\n", NumToClear);
			counter = 0;
			for(int zone = 0; zone < NUM_FREQ_ZONES; zone++){
				if(StateChart[zone].Is_ZeroCrossVel_Clear())	counter++;
			}
			if(counter < NumToClear)	{ printf("\t\tNG(%d/%d)\n", counter, NUM_FREQ_ZONES); b_DetectClap = false; }
			else						{ printf("\tOK(%d/%d)\n", counter, NUM_FREQ_ZONES); }

			/* */
			NumToClear = 2;
			printf("> ZeroCross Acc : %d\n", NumToClear);
			counter = 0;
			for(int zone = 0; zone < NUM_FREQ_ZONES; zone++){
				if(StateChart[zone].Is_ZeroCrossAcc_Clear())	counter++;
			}
			if(counter < NumToClear)	{ printf("\t\tNG(%d/%d)\n", counter, NUM_FREQ_ZONES); b_DetectClap = false; }
			else						{ printf("\tOK(%d/%d)\n", counter, NUM_FREQ_ZONES); }
			
			/* */
			NumToClear = 2;
			printf("> ZeroCross Jerk : %d\n", NumToClear);
			counter = 0;
			for(int zone = 0; zone < NUM_FREQ_ZONES; zone++){
				if(StateChart[zone].Is_ZeroCrossJerk_Clear())	counter++;
			}
			if(counter < NumToClear)	{ printf("\t\tNG(%d/%d)\n", counter, NUM_FREQ_ZONES); b_DetectClap = false; }
			else						{ printf("\tOK(%d/%d)\n", counter, NUM_FREQ_ZONES); }
			
			
			/********************
			********************/
			if(b_DetectClap){
				printf("\n<<<<< Detect >>>>>\n\n");
				
				/* */
				ofxOscMessage m;
				m.setAddress("/DetectClap");
				m.addIntArg(0); // dummy.
				for(int i = 0; i < NUM_OSC_TARGET; i++){
					Osc[i].OscSend.sendMessage(m);
				}

			}else{
				printf("\nXXXXX Failed XXXXX\n\n");
			}
			
			fflush(stdout);
			
		}else{
			b_DetectClap = false;
		}
		
	}else{
		b_DetectClap = false;
	}
	
	/********************
	********************/
	update_Array(Time_DetectClap, FBO_TIME_WIDTH, b_DetectClap);
}

/******************************
******************************/
int THREAD_FFT::fft(double x[], double y[], int IsReverse)
{
	/*****************
		bit反転
	*****************/
	int i, j;
	for(i = 0; i < N; i++){
		j = bitrev[i];
		if(i < j){
			double t;
			t = x[i]; x[i] = x[j]; x[j] = t;
			t = y[i]; y[i] = y[j]; y[j] = t;
		}
	}

	/*****************
		変換
	*****************/
	int n4 = N / 4;
	int k, ik, h, d, k2;
	double s, c, dx, dy;
	for(k = 1; k < N; k = k2){
		h = 0;
		k2 = k + k;
		d = N / k2;

		for(j = 0; j < k; j++){
			c = sintbl[h + n4];
			if(IsReverse)	s = -sintbl[h];
			else			s = sintbl[h];

			for(i = j; i < N; i += k2){
				ik = i + k;
				dx = s * y[ik] + c * x[ik];
				dy = c * y[ik] - s * x[ik];

				x[ik] = x[i] - dx;
				x[i] += dx;

				y[ik] = y[i] - dy;
				y[i] += dy;
			}
			h += d;
		}
	}

	/*****************
	*****************/
	if(!IsReverse){
		for(i = 0; i < N; i++){
			x[i] /= N;
			y[i] /= N;
		}
	}

	return 0;
}

/******************************
******************************/
void THREAD_FFT::make_bitrev(void)
{
	int i, j, k, n2;

	n2 = N / 2;
	i = j = 0;

	for(;;){
		bitrev[i] = j;
		if(++i >= N)	break;
		k = n2;
		while(k <= j)	{j -= k; k /= 2;}
		j += k;
	}
}

/******************************
******************************/
void THREAD_FFT::make_sintbl(void)
{
	for(int i = 0; i < N + N/4; i++){
		sintbl[i] = sin(2 * PI * i / N);
	}
}


