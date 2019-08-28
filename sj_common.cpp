/************************************************************
************************************************************/
#include "sj_common.h"

/************************************************************
************************************************************/
/********************
********************/
int GPIO_0 = 0;
int GPIO_1 = 0;

/********************
********************/
GUI_GLOBAL* Gui_Global = NULL;

FILE* fp_Log = NULL;
FILE* fp_Log_main = NULL;
FILE* fp_Log_fft = NULL;

bool b_PauseGraph = false;

OSC_TARGET Osc[NUM_OSC_TARGET];

/************************************************************
func
************************************************************/
/******************************
******************************/
double LPF(double LastVal, double CurrentVal, double Alpha_dt, double dt)
{
	double Alpha;
	if((Alpha_dt <= 0) || (Alpha_dt < dt))	Alpha = 1;
	else									Alpha = 1/Alpha_dt * dt;
	
	return CurrentVal * Alpha + LastVal * (1 - Alpha);
}

/******************************
******************************/
double LPF(double LastVal, double CurrentVal, double Alpha)
{
	if(Alpha < 0)		Alpha = 0;
	else if(1 < Alpha)	Alpha = 1;
	
	return CurrentVal * Alpha + LastVal * (1 - Alpha);
}

/******************************
******************************/
double sj_max(double a, double b)
{
	if(a < b)	return b;
	else		return a;
}


/************************************************************
class
************************************************************/

GUI_GLOBAL::GUI_GLOBAL()
{
}

/******************************
******************************/
void GUI_GLOBAL::ReadAndSet_PreSet__Scale()
{
	/********************
	********************/
	FILE* fp = NULL;
	fp = fopen("../../../data/GUI_Scale.txt", "r");
	if(fp == NULL) return;
	
	char buf[BUF_SIZE_S];
	while(fscanf(fp, "%s", buf) != EOF){
		if(strcmp(buf, "<GraphScale_FFTGain_Monitor>") == 0)			ReadAndSet_init_from_to(Paramset__GraphScale_FFTGain_Monitor, fp, buf);
		else if(strcmp(buf, "<GraphScale_FFTGain_Monitor_H>") == 0)		ReadAndSet_init_from_to(Paramset__GraphScale_FFTGain_Monitor_H, fp, buf);
		else if(strcmp(buf, "<GraphScale_FFTGain_Diff>") == 0)			ReadAndSet_init_from_to(Paramset__GraphScale_FFTGain_Diff, fp, buf);
		else if(strcmp(buf, "<GraphScale_FFTGain_Diff_H>") == 0)		ReadAndSet_init_from_to(Paramset__GraphScale_FFTGain_Diff_H, fp, buf);
		else if(strcmp(buf, "<GraphScale_Time_Raw[0]>") == 0)			ReadAndSet_init_from_to(Paramset__GraphScale_Time_Raw[0], fp, buf);
		else if(strcmp(buf, "<GraphScale_Time_Vel[0]>") == 0)			ReadAndSet_init_from_to(Paramset__GraphScale_Time_Vel[0], fp, buf);
		else if(strcmp(buf, "<GraphScale_Time_Acc[0]>") == 0)			ReadAndSet_init_from_to(Paramset__GraphScale_Time_Acc[0], fp, buf);
		else if(strcmp(buf, "<GraphScale_Time_Jerk[0]>") == 0)			ReadAndSet_init_from_to(Paramset__GraphScale_Time_Jerk[0], fp, buf);
		else if(strcmp(buf, "<thresh_raw_L[0]>") == 0)					ReadAndSet_init_from_to(Paramset__thresh_raw_L[0], fp, buf);
		else if(strcmp(buf, "<thresh_vel_H[0]>") == 0)					ReadAndSet_init_from_to(Paramset__thresh_vel_H[0], fp, buf);
		else if(strcmp(buf, "<thresh_acc_H[0]>") == 0)					ReadAndSet_init_from_to(Paramset__thresh_acc_H[0], fp, buf);
		else if(strcmp(buf, "<Hys_0cross_vel[0]>") == 0)				ReadAndSet_init_from_to(Paramset__Hys_0cross_vel[0], fp, buf);
		else if(strcmp(buf, "<Hys_0cross_acc[0]>") == 0)				ReadAndSet_init_from_to(Paramset__Hys_0cross_acc[0], fp, buf);
		else if(strcmp(buf, "<Hys_0cross_jerk[0]>") == 0)				ReadAndSet_init_from_to(Paramset__Hys_0cross_jerk[0], fp, buf);
		else if(strcmp(buf, "<GraphScale_Time_Raw[1]>") == 0)			ReadAndSet_init_from_to(Paramset__GraphScale_Time_Raw[1], fp, buf);
		else if(strcmp(buf, "<GraphScale_Time_Vel[1]>") == 0)			ReadAndSet_init_from_to(Paramset__GraphScale_Time_Vel[1], fp, buf);
		else if(strcmp(buf, "<GraphScale_Time_Acc[1]>") == 0)			ReadAndSet_init_from_to(Paramset__GraphScale_Time_Acc[1], fp, buf);
		else if(strcmp(buf, "<GraphScale_Time_Jerk[1]>") == 0)			ReadAndSet_init_from_to(Paramset__GraphScale_Time_Jerk[1], fp, buf);
		else if(strcmp(buf, "<thresh_raw_L[1]>") == 0)					ReadAndSet_init_from_to(Paramset__thresh_raw_L[1], fp, buf);
		else if(strcmp(buf, "<thresh_vel_H[1]>") == 0)					ReadAndSet_init_from_to(Paramset__thresh_vel_H[1], fp, buf);
		else if(strcmp(buf, "<thresh_acc_H[1]>") == 0)					ReadAndSet_init_from_to(Paramset__thresh_acc_H[1], fp, buf);
		else if(strcmp(buf, "<Hys_0cross_vel[1]>") == 0)				ReadAndSet_init_from_to(Paramset__Hys_0cross_vel[1], fp, buf);
		else if(strcmp(buf, "<Hys_0cross_acc[1]>") == 0)				ReadAndSet_init_from_to(Paramset__Hys_0cross_acc[1], fp, buf);
		else if(strcmp(buf, "<Hys_0cross_jerk[1]>") == 0)				ReadAndSet_init_from_to(Paramset__Hys_0cross_jerk[1], fp, buf);
		else if(strcmp(buf, "<GraphScale_Time_Raw[2]>") == 0)			ReadAndSet_init_from_to(Paramset__GraphScale_Time_Raw[2], fp, buf);
		else if(strcmp(buf, "<GraphScale_Time_Vel[2]>") == 0)			ReadAndSet_init_from_to(Paramset__GraphScale_Time_Vel[2], fp, buf);
		else if(strcmp(buf, "<GraphScale_Time_Acc[2]>") == 0)			ReadAndSet_init_from_to(Paramset__GraphScale_Time_Acc[2], fp, buf);
		else if(strcmp(buf, "<GraphScale_Time_Jerk[2]>") == 0)			ReadAndSet_init_from_to(Paramset__GraphScale_Time_Jerk[2], fp, buf);
		else if(strcmp(buf, "<thresh_raw_L[2]>") == 0)					ReadAndSet_init_from_to(Paramset__thresh_raw_L[2], fp, buf);
		else if(strcmp(buf, "<thresh_vel_H[2]>") == 0)					ReadAndSet_init_from_to(Paramset__thresh_vel_H[2], fp, buf);
		else if(strcmp(buf, "<thresh_acc_H[2]>") == 0)					ReadAndSet_init_from_to(Paramset__thresh_acc_H[2], fp, buf);
		else if(strcmp(buf, "<Hys_0cross_vel[2]>") == 0)				ReadAndSet_init_from_to(Paramset__Hys_0cross_vel[2], fp, buf);
		else if(strcmp(buf, "<Hys_0cross_acc[2]>") == 0)				ReadAndSet_init_from_to(Paramset__Hys_0cross_acc[2], fp, buf);
		else if(strcmp(buf, "<Hys_0cross_jerk[2]>") == 0)				ReadAndSet_init_from_to(Paramset__Hys_0cross_jerk[2], fp, buf);
	}
	
	printf("Preset file(Scale) applied\n");
	fflush(stdout);
	
	fclose(fp);
}

/******************************
******************************/
void GUI_GLOBAL::ReadAndSet_PreSet__Param()
{
	/********************
	********************/
	FILE* fp = NULL;
	fp = fopen("../../../data/GUI_Param.txt", "r");
	if(fp == NULL) return;
	
	char buf[BUF_SIZE_S];
	while(fscanf(fp, "%s", buf) != EOF){
		if(strcmp(buf, "<MaxError_0cross_vel[0]>") == 0)			ReadAndSet_init_from_to(Paramset__MaxError_0cross_vel[0], fp, buf);
		else if(strcmp(buf, "<MaxError_0cross_acc[0]>") == 0)		ReadAndSet_init_from_to(Paramset__MaxError_0cross_acc[0], fp, buf);
		else if(strcmp(buf, "<MaxError_0cross_jerk[0]>") == 0)		ReadAndSet_init_from_to(Paramset__MaxError_0cross_jerk[0], fp, buf);
		else if(strcmp(buf, "<MaxError_0cross_vel[1]>") == 0)		ReadAndSet_init_from_to(Paramset__MaxError_0cross_vel[1], fp, buf);
		else if(strcmp(buf, "<MaxError_0cross_acc[1]>") == 0)		ReadAndSet_init_from_to(Paramset__MaxError_0cross_acc[1], fp, buf);
		else if(strcmp(buf, "<MaxError_0cross_jerk[1]>") == 0)		ReadAndSet_init_from_to(Paramset__MaxError_0cross_jerk[1], fp, buf);
		else if(strcmp(buf, "<MaxError_0cross_vel[2]>") == 0)		ReadAndSet_init_from_to(Paramset__MaxError_0cross_vel[2], fp, buf);
		else if(strcmp(buf, "<MaxError_0cross_acc[2]>") == 0)		ReadAndSet_init_from_to(Paramset__MaxError_0cross_acc[2], fp, buf);
		else if(strcmp(buf, "<MaxError_0cross_jerk[2]>") == 0)		ReadAndSet_init_from_to(Paramset__MaxError_0cross_jerk[2], fp, buf);
	}
	
	printf("Preset file(Param) applied\n");
	fflush(stdout);
	
	fclose(fp);
}

/******************************
******************************/
void GUI_GLOBAL::ReadAndSet_init_from_to(GUI_PARAM_SET_FLOAT& ParamSet, FILE* fp, char* buf)
{
	float initial, from, to;
	
	fscanf(fp, "%s", buf);	initial	= atof(buf);
	fscanf(fp, "%s", buf);	from	= atof(buf);
	fscanf(fp, "%s", buf);	to		= atof(buf);
	
	ParamSet.set(initial, from, to);
}

/******************************
******************************/
void GUI_GLOBAL::setup(string GuiName, string FileName, float x, float y)
{
	/********************
	********************/
	ReadAndSet_PreSet__Scale();
	ReadAndSet_PreSet__Param();
	
	
	/********************
	********************/
	gui.setup(GuiName.c_str(), FileName.c_str(), x, y);
	
	/********************
	********************/
	Group_LPF.setup("LPF");
		Group_LPF.add(LPF_dt__FFTGain.setup("FFT", 0.05, 0, 0.2));
		Group_LPF.add(LPF_dt__Time_Raw.setup("t_Raw", 0, 0, 0.2));
		Group_LPF.add(LPF_dt__Time_Vel.setup("t_Vel", 0, 0, 0.2));
		Group_LPF.add(LPF_dt__Time_Acc.setup("t_Acc", 0, 0, 0.2));
		Group_LPF.add(LPF_dt__Time_Jerk.setup("t_Jerk", 0, 0, 0.2));
		// Group_LPF.add(LPF_dt__Time_Jerk.setup("t_Jerk", 0.01, 0, 0.2));
	gui.add(&Group_LPF);
	
	Group_Zone.setup("Zone");
		Group_Zone.add(Zone_FreqFrom[0].setup("0:from", 24, 0, 511));
		// Group_Zone.add(Zone_FreqWidth[0].setup("0:width", 45, 0, 511));
		Group_Zone.add(Zone_FreqWidth[0].setup("0:width", 55, 0, 511));
		Group_Zone.add(CancelLineIn_Gain[0].setup("0:CancelGain", 1, 0, 2));

		Group_Zone.add(Zone_FreqFrom[1].setup("1:from", 150, 0, 511));
		Group_Zone.add(Zone_FreqWidth[1].setup("1:width", 350, 0, 511));
		Group_Zone.add(CancelLineIn_Gain[1].setup("1:CancelGain", 1, 0, 2));

		Group_Zone.add(Zone_FreqFrom[2].setup("2:from", 350, 0, 511));
		Group_Zone.add(Zone_FreqWidth[2].setup("2:width", 150, 0, 511));
		Group_Zone.add(CancelLineIn_Gain[2].setup("2:CancelGain", 1, 0, 2));
	gui.add(&Group_Zone);
		
	Group_GraphScale_FFT.setup("GraphScale_FFT");
		Group_GraphScale_FFT.add(GraphScale_Zoom_FreqFrom.setup("Zoom from", 100, 0, 511));						
								
		Group_GraphScale_FFT.add(GraphScale_FFTGain_Monitor.setup("Scale_Monitor", Paramset__GraphScale_FFTGain_Monitor.initial, Paramset__GraphScale_FFTGain_Monitor.from, Paramset__GraphScale_FFTGain_Monitor.to ));
		Group_GraphScale_FFT.add(GraphScale_FFTGain_Monitor_H.setup("Scale_Monitor_H", Paramset__GraphScale_FFTGain_Monitor_H.initial, Paramset__GraphScale_FFTGain_Monitor_H.from, Paramset__GraphScale_FFTGain_Monitor_H.to ));
								
		Group_GraphScale_FFT.add(GraphScale_FFTGain_Diff.setup("Scale_Diff", Paramset__GraphScale_FFTGain_Diff.initial, Paramset__GraphScale_FFTGain_Diff.from, Paramset__GraphScale_FFTGain_Diff.to ));
		Group_GraphScale_FFT.add(GraphScale_FFTGain_Diff_H.setup("Scale_Diff_H", Paramset__GraphScale_FFTGain_Diff_H.initial, Paramset__GraphScale_FFTGain_Diff_H.from, Paramset__GraphScale_FFTGain_Diff_H.to ));
	gui.add(&Group_GraphScale_FFT);
	
	Group_GraphScale_Time.setup("GraphScale_Time");
		Group_GraphScale_Time.add(GraphScale_Time_Raw[0].setup("0:Scale_Raw", Paramset__GraphScale_Time_Raw[0].initial, Paramset__GraphScale_Time_Raw[0].from, Paramset__GraphScale_Time_Raw[0].to ));
		Group_GraphScale_Time.add(GraphScale_Time_Vel[0].setup("0:Scale_Vel", Paramset__GraphScale_Time_Vel[0].initial, Paramset__GraphScale_Time_Vel[0].from, Paramset__GraphScale_Time_Vel[0].to ));
		Group_GraphScale_Time.add(GraphScale_Time_Acc[0].setup("0:Scale_Acc", Paramset__GraphScale_Time_Acc[0].initial, Paramset__GraphScale_Time_Acc[0].from, Paramset__GraphScale_Time_Acc[0].to ));
		Group_GraphScale_Time.add(GraphScale_Time_Jerk[0].setup("0:Scale_Jerk", Paramset__GraphScale_Time_Jerk[0].initial, Paramset__GraphScale_Time_Jerk[0].from, Paramset__GraphScale_Time_Jerk[0].to ));
								
		Group_GraphScale_Time.add(GraphScale_Time_Raw[1].setup("1:Scale_Raw", Paramset__GraphScale_Time_Raw[1].initial, Paramset__GraphScale_Time_Raw[1].from, Paramset__GraphScale_Time_Raw[1].to ));
		Group_GraphScale_Time.add(GraphScale_Time_Vel[1].setup("1:Scale_Vel", Paramset__GraphScale_Time_Vel[1].initial, Paramset__GraphScale_Time_Vel[1].from, Paramset__GraphScale_Time_Vel[1].to ));
		Group_GraphScale_Time.add(GraphScale_Time_Acc[1].setup("1:Scale_Acc", Paramset__GraphScale_Time_Acc[1].initial, Paramset__GraphScale_Time_Acc[1].from, Paramset__GraphScale_Time_Acc[1].to ));
		Group_GraphScale_Time.add(GraphScale_Time_Jerk[1].setup("1:Scale_Jerk", Paramset__GraphScale_Time_Jerk[1].initial, Paramset__GraphScale_Time_Jerk[1].from, Paramset__GraphScale_Time_Jerk[1].to ));
								
		Group_GraphScale_Time.add(GraphScale_Time_Raw[2].setup("2:Scale_Raw", Paramset__GraphScale_Time_Raw[2].initial, Paramset__GraphScale_Time_Raw[2].from, Paramset__GraphScale_Time_Raw[2].to ));
		Group_GraphScale_Time.add(GraphScale_Time_Vel[2].setup("2:Scale_Vel", Paramset__GraphScale_Time_Vel[2].initial, Paramset__GraphScale_Time_Vel[2].from, Paramset__GraphScale_Time_Vel[2].to ));
		Group_GraphScale_Time.add(GraphScale_Time_Acc[2].setup("2:Scale_Acc", Paramset__GraphScale_Time_Acc[2].initial, Paramset__GraphScale_Time_Acc[2].from, Paramset__GraphScale_Time_Acc[2].to ));
		Group_GraphScale_Time.add(GraphScale_Time_Jerk[2].setup("2:Scale_Jerk", Paramset__GraphScale_Time_Jerk[2].initial, Paramset__GraphScale_Time_Jerk[2].from, Paramset__GraphScale_Time_Jerk[2].to ));
	gui.add(&Group_GraphScale_Time);
		
	Group_thresh.setup("thresh");
		Group_thresh.add(thresh_raw_L[0].setup("0:th_raw_L", Paramset__thresh_raw_L[0].initial, Paramset__thresh_raw_L[0].from, Paramset__thresh_raw_L[0].to));
		Group_thresh.add(thresh_raw_L[1].setup("1:th_raw_L", Paramset__thresh_raw_L[1].initial, Paramset__thresh_raw_L[1].from, Paramset__thresh_raw_L[1].to));
		Group_thresh.add(thresh_raw_L[2].setup("2:th_raw_L", Paramset__thresh_raw_L[2].initial, Paramset__thresh_raw_L[2].from, Paramset__thresh_raw_L[2].to));
		
		Group_thresh.add(thresh_vel_H[0].setup("0:th_vel_H", Paramset__thresh_vel_H[0].initial, Paramset__thresh_vel_H[0].from, Paramset__thresh_vel_H[0].to));
		Group_thresh.add(thresh_vel_H[1].setup("1:th_vel_H", Paramset__thresh_vel_H[1].initial, Paramset__thresh_vel_H[1].from, Paramset__thresh_vel_H[1].to));
		Group_thresh.add(thresh_vel_H[2].setup("2:th_vel_H", Paramset__thresh_vel_H[2].initial, Paramset__thresh_vel_H[2].from, Paramset__thresh_vel_H[2].to));
		
		Group_thresh.add(thresh_acc_H[0].setup("0:th_acc_H", Paramset__thresh_acc_H[0].initial, Paramset__thresh_acc_H[0].from, Paramset__thresh_acc_H[0].to));
		Group_thresh.add(thresh_acc_H[1].setup("1:th_acc_H", Paramset__thresh_acc_H[1].initial, Paramset__thresh_acc_H[1].from, Paramset__thresh_acc_H[1].to));
		Group_thresh.add(thresh_acc_H[2].setup("2:th_acc_H", Paramset__thresh_acc_H[2].initial, Paramset__thresh_acc_H[2].from, Paramset__thresh_acc_H[2].to));
	gui.add(&Group_thresh);
	
	Group_Hys_0cross.setup("Hys 0cross");
		Group_Hys_0cross.add(Hys_0cross_vel[0].setup("0:Hys_vel", Paramset__Hys_0cross_vel[0].initial, Paramset__Hys_0cross_vel[0].from, Paramset__Hys_0cross_vel[0].to));
		Group_Hys_0cross.add(Hys_0cross_vel[1].setup("1:Hys_vel", Paramset__Hys_0cross_vel[1].initial, Paramset__Hys_0cross_vel[1].from, Paramset__Hys_0cross_vel[1].to));
		Group_Hys_0cross.add(Hys_0cross_vel[2].setup("2:Hys_vel", Paramset__Hys_0cross_vel[2].initial, Paramset__Hys_0cross_vel[2].from, Paramset__Hys_0cross_vel[2].to));
		
		Group_Hys_0cross.add(Hys_0cross_acc[0].setup("0:Hys_acc", Paramset__Hys_0cross_acc[0].initial, Paramset__Hys_0cross_acc[0].from, Paramset__Hys_0cross_acc[0].to));
		Group_Hys_0cross.add(Hys_0cross_acc[1].setup("1:Hys_acc", Paramset__Hys_0cross_acc[1].initial, Paramset__Hys_0cross_acc[1].from, Paramset__Hys_0cross_acc[1].to));
		Group_Hys_0cross.add(Hys_0cross_acc[2].setup("2:Hys_acc", Paramset__Hys_0cross_acc[2].initial, Paramset__Hys_0cross_acc[2].from, Paramset__Hys_0cross_acc[2].to));
		
		Group_Hys_0cross.add(Hys_0cross_jerk[0].setup("0:Hys_jerk", Paramset__Hys_0cross_jerk[0].initial, Paramset__Hys_0cross_jerk[0].from, Paramset__Hys_0cross_jerk[0].to));
		Group_Hys_0cross.add(Hys_0cross_jerk[1].setup("1:Hys_jerk", Paramset__Hys_0cross_jerk[1].initial, Paramset__Hys_0cross_jerk[1].from, Paramset__Hys_0cross_jerk[1].to));
		Group_Hys_0cross.add(Hys_0cross_jerk[2].setup("2:Hys_jerk", Paramset__Hys_0cross_jerk[2].initial, Paramset__Hys_0cross_jerk[2].from, Paramset__Hys_0cross_jerk[2].to));
	gui.add(&Group_Hys_0cross);
	
	Group_MaxError_ZeroCross.setup("MaxError 0cross");
		Group_MaxError_ZeroCross.add(MaxError_0cross_vel[0].setup("0:Error_vel", Paramset__MaxError_0cross_vel[0].initial, Paramset__MaxError_0cross_vel[0].from, Paramset__MaxError_0cross_vel[0].to));
		Group_MaxError_ZeroCross.add(MaxError_0cross_vel[1].setup("1:Error_vel", Paramset__MaxError_0cross_vel[1].initial, Paramset__MaxError_0cross_vel[1].from, Paramset__MaxError_0cross_vel[1].to));
		Group_MaxError_ZeroCross.add(MaxError_0cross_vel[2].setup("2:Error_vel", Paramset__MaxError_0cross_vel[2].initial, Paramset__MaxError_0cross_vel[2].from, Paramset__MaxError_0cross_vel[2].to));
		
		Group_MaxError_ZeroCross.add(MaxError_0cross_acc[0].setup("0:Error_acc", Paramset__MaxError_0cross_acc[0].initial, Paramset__MaxError_0cross_acc[0].from, Paramset__MaxError_0cross_acc[0].to));
		Group_MaxError_ZeroCross.add(MaxError_0cross_acc[1].setup("1:Error_acc", Paramset__MaxError_0cross_acc[1].initial, Paramset__MaxError_0cross_acc[1].from, Paramset__MaxError_0cross_acc[1].to));
		Group_MaxError_ZeroCross.add(MaxError_0cross_acc[2].setup("2:Error_acc", Paramset__MaxError_0cross_acc[2].initial, Paramset__MaxError_0cross_acc[2].from, Paramset__MaxError_0cross_acc[2].to));
		
		Group_MaxError_ZeroCross.add(MaxError_0cross_jerk[0].setup("0:Error_jerk", Paramset__MaxError_0cross_jerk[0].initial, Paramset__MaxError_0cross_jerk[0].from, Paramset__MaxError_0cross_jerk[0].to));
		Group_MaxError_ZeroCross.add(MaxError_0cross_jerk[1].setup("1:Error_jerk", Paramset__MaxError_0cross_jerk[1].initial, Paramset__MaxError_0cross_jerk[1].from, Paramset__MaxError_0cross_jerk[1].to));
		Group_MaxError_ZeroCross.add(MaxError_0cross_jerk[2].setup("2:Error_jerk", Paramset__MaxError_0cross_jerk[2].initial, Paramset__MaxError_0cross_jerk[2].from, Paramset__MaxError_0cross_jerk[2].to));
	gui.add(&Group_MaxError_ZeroCross);
		
	/********************
	********************/
	gui.minimizeAll();
}

