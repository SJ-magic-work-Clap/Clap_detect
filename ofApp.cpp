/************************************************************
************************************************************/
#include "ofApp.h"
#include <time.h>


/************************************************************
************************************************************/

/******************************
******************************/
ofApp::ofApp(int _soundStream_Input_DeviceId, int _soundStream_Output_DeviceId)
: col_Black(ofColor(0, 0, 0))
, col_White(ofColor(255, 255, 255))
, col_Red(ofColor(255, 0, 0))
, col_Green(ofColor(0, 255, 0))
, col_Blue(ofColor(0, 0, 255))
, col_Yellow(ofColor(255, 255, 0))
, col_Orange(ofColor(255, 150, 0))
, col_Pink(ofColor(255, 0, 255))
, col_Cian(ofColor(0, 255, 255))
, col_Sky(ofColor(0, 180, 255))
, col_Back0(ofColor(0))
, col_Back1(ofColor(30))

, soundStream_Input_DeviceId(_soundStream_Input_DeviceId)
, soundStream_Output_DeviceId(_soundStream_Output_DeviceId)
, b_DispGui(true)
, fft_thread(THREAD_FFT::getInstance())
, png_id(0)
// , b_PauseGraph(false)
, ofs_x_ReadCursor(0)
{
	/********************
	********************/
	fp_Log = fopen("../../../data/Log.csv", "w");
	fp_Log_main = fopen("../../../data/Log_main.csv", "w");
	fp_Log_fft = fopen("../../../data/Log_fft.csv", "w");
	
	/********************
	********************/
	srand((unsigned) time(NULL));
	
	/********************
	********************/
	font[FONT_S].load("font/RictyDiminished-Regular.ttf", 8, true, true, true);
	font[FONT_M].load("font/RictyDiminished-Regular.ttf", 15, true, true, true);
	font[FONT_L].load("font/RictyDiminished-Regular.ttf", 25, true, true, true);
	
	/********************
	********************/
	Osc[OSC_TARGET__VIDEO].setup("127.0.0.1", 12345, 12346);
	Osc[OSC_TARGET__CLAPON].setup("127.0.0.1", 12349, 12350);
	Osc[OSC_TARGET__STROBE].setup("127.0.0.1", 12347, 12348);
	
	/********************
	********************/
	Vboset_Monitor.setup(AUDIO_BUF_SIZE/2 * 2); // GL_LINES
	Vboset_Linein.setup(AUDIO_BUF_SIZE/2 * 2); // GL_LINES
	Vboset_Diff.setup(AUDIO_BUF_SIZE/2 * 2); // GL_LINES
	
	for(int zone = 0; zone < NUM_FREQ_ZONES; zone++){
		for(int DataType = 0; DataType < NUM_TIMEDATA; DataType++){
			Vboset_Time[zone][DataType].setup(FBO_TIME_WIDTH * 1);
		}
	}
	Vboset_DetectClap.setup(FBO_TIME_WIDTH * 1);
	Vboset_AdjustScale.setup(FBO_TIME_WIDTH * 1);
	/********************
	Vboset_Timeは、slideしながら表示なので、zeroでの初期化が必要だが、setup()で zero clearするようにしたので、ok.
	********************/
}

/******************************
******************************/
ofApp::~ofApp()
{
	fclose(fp_Log);
	fclose(fp_Log_main);
	fclose(fp_Log_fft);
}

/******************************
******************************/
void ofApp::exit()
{
	/********************
	ofAppとaudioが別threadなので、ここで止めておくのが安全.
	********************/
	soundStream.stop();
	soundStream.close();
	
	/********************
	********************/
	fft_thread->exit();
	try{
		/********************
		stop済みのthreadをさらにstopすると、Errorが出るようだ。
		********************/
		while(fft_thread->isThreadRunning()){
			fft_thread->waitForThread(true);
		}
		
	}catch(...){
		printf("Thread exiting Error\n");
	}
	
	
	/********************
	********************/
	SaveGuiParam_toFile();
	
	/********************
	********************/
	printf("\n> Good bye\n");
}


//--------------------------------------------------------------
void ofApp::setup(){
	/********************
	********************/
	ofSetWindowTitle("CLAP");
	ofSetVerticalSync(true);
	ofSetFrameRate(30); // Clap detectは、th_fft内で完了. 本threadは表示のみなので、遅くてok.
	ofSetWindowShape(WINDOW_WIDTH, WINDOW_HEIGHT);
	ofSetEscapeQuitsApp(false);
	
	/*
	ofEnableAlphaBlending();
	ofEnableBlendMode(OF_BLENDMODE_ALPHA);
	// ofEnableBlendMode(OF_BLENDMODE_ADD);
	// ofEnableSmoothing();
	*/
	
	/********************
	********************/
	setup_Gui();
	
	/********************
	********************/
	AudioSample.resize(AUDIO_BUF_SIZE);
	
	fft_thread->setup(); // audioIn()/audioOut()がstartする前に初期化しておかないと、不正accessが発生する.
	
	/********************
	********************/
	Refresh_BarColor();
	RefreshVerts();
	
	/********************
	********************/
	// fbo_FFT.allocate(FBO_FFT_WIDTH, FBO_FFT_HEIGHT, GL_RGBA, 4);
	fbo_FFT.allocate(FBO_FFT_WIDTH, FBO_FFT_HEIGHT, GL_RGBA, 0);
	Clear_fbo(fbo_FFT);
	
	fbo_FFT_Diff.allocate(FBO_FFT_WIDTH, FBO_FFT_HEIGHT, GL_RGBA, 0);
	Clear_fbo(fbo_FFT_Diff);
	
	for(int zone = 0; zone < NUM_FREQ_ZONES; zone++){
		for(int DataType = 0; DataType < NUM_TIMEDATA; DataType++){
			fbo_Time[zone][DataType].allocate(FBO_TIME_WIDTH, FBO_TIME_HEIGHT, GL_RGBA, 0);
			Clear_fbo(fbo_Time[zone][DataType]);
		}
	}
	
	/********************
	settings.setInListener(this);
	settings.setOutListener(this);
	settings.sampleRate = 44100;
	settings.numInputChannels = 2;
	settings.numOutputChannels = 2;
	settings.bufferSize = bufferSize;
	
	soundStream.setup(settings);
	********************/
	soundStream.printDeviceList();
	
	/********************
	soundStream.setup()の位置に注意:最後
		setup直後、audioIn()/audioOut()がstartする.
		これらのmethodは、fft_threadにaccessするので、start前にReStart()によって、fft_threadが初期化されていないと、不正accessが発生してしまう.
	********************/
	setup_SoundStream();
}

/******************************
******************************/
void ofApp::Clear_fbo(ofFbo& fbo)
{
	fbo.begin();
	
	// Clear with alpha, so we can capture via syphon and composite elsewhere should we want.
	glClearColor(0.0, 0.0, 0.0, 0.0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	
	ofClear(0, 0, 0, 0);
	
	fbo.end();
}

/******************************
******************************/
void ofApp::setup_SoundStream()
{
	/********************
	********************/
	vector<ofSoundDevice> devices = soundStream.getDeviceList();
	
	/********************
	search by device name.
	********************/
	if(soundStream_Input_DeviceId == -2){
		int i;
		for(i = 0; i < devices.size(); i++){
			if(devices[i].name == "Focusrite Audio Engineering Ltd.: Focusrite Thunderbolt" ){
				soundStream_Input_DeviceId = i;
				soundStream_Output_DeviceId = -1;
				break;
			}
		}
		
		if(i == devices.size()){
			b_AudioDeviceSearchFailed = true;
			t_AudioDeviceSearchFailed = ofGetElapsedTimeMillis();
			
			return; // Audio not start.
		}
	}
	
	
	/********************
	settings.setInListener(this);
	settings.setOutListener(this);
	settings.sampleRate = 44100;
	settings.numInputChannels = 2;
	settings.numOutputChannels = 2;
	settings.bufferSize = bufferSize;
	
	soundStream.setup(settings);
	********************/
	ofSoundStreamSettings settings;
	
	if( soundStream_Input_DeviceId == -1 ){
		ofExit();
		return;
		
	}else{
		
		if( soundStream_Input_DeviceId != -1 ){
			settings.setInDevice(devices[soundStream_Input_DeviceId]);
			settings.setInListener(this);
			settings.numInputChannels = NUM_AUDIO_CHS;
		}else{
			settings.numInputChannels = 0;
		}
		
		if( soundStream_Output_DeviceId != -1 ){
			if(devices[soundStream_Output_DeviceId].name == "Apple Inc.: Built-in Output"){
				printf("!!!!! prohibited to use [%s] for output ... by SJ for safety !!!!!\n", devices[soundStream_Output_DeviceId].name.c_str());
				fflush(stdout);
				
				settings.numOutputChannels = 0;
				
			}else{
				settings.setOutDevice(devices[soundStream_Output_DeviceId]);
				settings.numOutputChannels = NUM_AUDIO_CHS;
				settings.setOutListener(this); /* Don't forget this */
			}
		}else{
			settings.numOutputChannels = 0;
		}
		
		settings.numBuffers = AUDIO_BUFFERS;
		settings.sampleRate = AUDIO_SAMPLERATE;
		settings.bufferSize = AUDIO_BUF_SIZE;
	}
	
	/********************
	soundStream.setup()の位置に注意:最後
		setup直後、audioIn()/audioOut()がstartする.
		これらのmethodは、fft_threadにaccessするので、start前にReStart()によって、fft_threadが初期化されていないと、不正accessが発生してしまう.
	********************/
	soundStream.setup(settings);
	// soundStream.start();
}

/******************************
description
	memoryを確保は、app start後にしないと、
	segmentation faultになってしまった。
******************************/
void ofApp::setup_Gui()
{
	/********************
	********************/
	Gui_Global = new GUI_GLOBAL;
	Gui_Global->setup("Param", "gui.xml", 1300, 10);
}

/******************************
******************************/
void ofApp::Refresh_BarColor()
{
	/********************
	********************/
	Vboset_Monitor.set_singleColor(col_Green.get_col(180));
	Vboset_Linein.set_singleColor(col_White.get_col(180));
	Vboset_Diff.set_singleColor(col_White.get_col(180));
	
	for(int zone = 0; zone < NUM_FREQ_ZONES; zone++){
		for(int DataType = 0; DataType < NUM_TIMEDATA; DataType++){
			Vboset_Time[zone][DataType].set_singleColor(col_White.get_col(180));
		}
	}
	Vboset_DetectClap.set_singleColor(col_Red.get_col(180));
	
	Vboset_AdjustScale.set_singleColor(col_Green.get_col(180));
}

/******************************
******************************/
void ofApp::RefreshVerts()
{
	float BarWidth = GRAPH_BAR_WIDTH__FFT_GAIN;
	float BarSpace = GRAPH_BAR_SPACE__FFT_GAIN;
	
	/********************
	********************/
	for(int i = 0; i < AUDIO_BUF_SIZE/2; i++){
		float GraphScale;
		if(i < Gui_Global->GraphScale_Zoom_FreqFrom)	GraphScale = Gui_Global->GraphScale_FFTGain_Monitor;
		else											GraphScale = Gui_Global->GraphScale_FFTGain_Monitor_H;
		
		Vboset_Monitor.VboVerts[i * 2 + 0].set( BarSpace * i + BarWidth * 0 , 0 );
		Vboset_Monitor.VboVerts[i * 2 + 1].set( BarSpace * i + BarWidth * 0 , fft_thread->getVal__FreqArray_x_GraphScale(0/*ArrayId*/, i, GraphScale, FBO_FFT_HEIGHT, false) );
	}
	
	for(int i = 0; i < AUDIO_BUF_SIZE/2; i++){
		float GraphScale;
		if(i < Gui_Global->GraphScale_Zoom_FreqFrom)	GraphScale = Gui_Global->GraphScale_FFTGain_Monitor;
		else											GraphScale = Gui_Global->GraphScale_FFTGain_Monitor_H;
		
		Vboset_Linein.VboVerts[i * 2 + 0].set( BarSpace * i + BarWidth * 1 , 0 );
		Vboset_Linein.VboVerts[i * 2 + 1].set( BarSpace * i + BarWidth * 1 , fft_thread->getVal__FreqArray_x_GraphScale(1/*ArrayId*/, i, GraphScale, FBO_FFT_HEIGHT, false) );
	}
	
	for(int i = 0; i < AUDIO_BUF_SIZE/2; i++){
		float GraphScale;
		if(i < Gui_Global->GraphScale_Zoom_FreqFrom)	GraphScale = Gui_Global->GraphScale_FFTGain_Diff;
		else											GraphScale = Gui_Global->GraphScale_FFTGain_Diff_H;
		
		Vboset_Diff.VboVerts[i * 2 + 0].set( BarSpace * i + BarWidth * 0 , 0 );
		Vboset_Diff.VboVerts[i * 2 + 1].set( BarSpace * i + BarWidth * 0 , fft_thread->getVal__FreqArray_x_GraphScale(2/*ArrayId*/, i, GraphScale, FBO_FFT_HEIGHT/2, false) );
	}
	
	/********************
	********************/
	for(int i = 0; i < FBO_TIME_WIDTH; i++){
		Vboset_AdjustScale.VboVerts[i].set(i, fft_thread->getVal__AdjustScaleId_x_ScreenHeight(i, FBO_TIME_HEIGHT));
	}
	
	/********************
	********************/
	if(!b_PauseGraph){
		for(int zone = 0; zone < NUM_FREQ_ZONES; zone++){
			for(int DataType = 0; DataType < NUM_TIMEDATA; DataType++){
				float GraphScale;
				float ScreenHeight = FBO_TIME_HEIGHT/2;
				
				if(DataType == 0)		{ GraphScale = Gui_Global->GraphScale_Time_Raw[zone]; ScreenHeight = FBO_TIME_HEIGHT; }
				else if(DataType == 1)	{ GraphScale = Gui_Global->GraphScale_Time_Vel[zone]; }
				else if(DataType == 2)	{ GraphScale = Gui_Global->GraphScale_Time_Acc[zone]; }
				else if(DataType == 3)	{ GraphScale = Gui_Global->GraphScale_Time_Jerk[zone]; }
				else if(DataType == 4)	{ GraphScale = STATE_CHART::get_NumStates(); /* Stateのmaxをset. */ }
				
				for(int i = 0; i < FBO_TIME_WIDTH; i++){
					Vboset_Time[zone][DataType].VboVerts[i].set(i, fft_thread->getVal__TimeArray_x_GraphScale(zone, DataType, i, GraphScale, ScreenHeight, false));
				}
			}
		}
		
		for(int i = 0; i < FBO_TIME_WIDTH; i++){
			if(fft_thread->getVal__TimeArray_DetectClap(i))		Vboset_DetectClap.VboVerts[i].set(i, FBO_TIME_HEIGHT * 9/10 /* 少し低くしておく */);
			else												Vboset_DetectClap.VboVerts[i].set(i, FBO_TIME_HEIGHT * 1/2 /* Base LineはHalf level */);
		}
	}
}

/******************************
******************************/
void ofApp::ReverseFromVbo(char* buf, const VBO_SET& Vboset, int id, float ScreenHeight, float GraphScale)
{
	if( (id < 0) || (Vboset.VboVerts.size() <= id) ){
		sprintf(buf, "---");
	}else{
		ofVec3f VboVal = Vboset.VboVerts[id];
		double val = ofMap(VboVal.y, 0, ScreenHeight, 0, GraphScale, false);
		sprintf(buf, "%+e", val);
	}
}

/******************************
******************************/
bool ofApp::checkif_FileExist(const char* FileName)
{
	if(ofFile::doesFileExist(FileName)){
		printf("loaded file of %s\n", FileName);
		return true;
		
	}else{
		printf("%s not exist\n", FileName);
		return false;
	}
}

/******************************
******************************/
void ofApp::update(){
	/********************
	********************/
	if(b_AudioDeviceSearchFailed){
		if(2000 < ofGetElapsedTimeMillis() - t_AudioDeviceSearchFailed){
			ofExit(1);
			return;
		}else{
			return;
		}
	}

	/********************
	********************/
	for(int i = 0; i < NUM_OSC_TARGET; i++){
		while(Osc[i].OscReceive.hasWaitingMessages()){
			ofxOscMessage m_receive;
			Osc[i].OscReceive.getNextMessage(m_receive); // 読み捨て 
		}
	}
	
	/********************
	********************/
	fft_thread->update();
}

/******************************
******************************/
void ofApp::draw(){
	/********************
	********************/
	if(b_AudioDeviceSearchFailed){
		ofBackground(0);
		ofSetColor(255, 0, 0, 255);
		
		char buf[BUF_SIZE_S];
		sprintf(buf, "Audio I/F not Exsist");
		font[FONT_L].drawString(buf, ofGetWidth()/2 - font[FONT_L].stringWidth(buf)/2, ofGetHeight()/2);
		
		return;
	}
	
	/********************
	********************/
	RefreshVerts();
	Refresh_BarColor();
	
	/********************
	以下は、audioOutからの呼び出しだと segmentation fault となってしまった.
	********************/
	Vboset_Monitor.update();
	Vboset_Linein.update();
	Vboset_Diff.update();
	
	for(int zone = 0; zone < NUM_FREQ_ZONES; zone++){
		for(int DataType = 0; DataType < NUM_TIMEDATA; DataType++){
			Vboset_Time[zone][DataType].update();
		}
	}
	Vboset_DetectClap.update();
	
	Vboset_AdjustScale.update();
	
	/********************
	********************/
	drawFbo_FFT();
	drawFbo_FFT_Diff();
	
	for(int zone = 0; zone < NUM_FREQ_ZONES; zone++){
		for(int DataType = 0; DataType < NUM_TIMEDATA; DataType++){
			/* */
			ofVec2f _translate;
			float y_max_Screen;
			
			if(DataType == 0)		{ _translate = ofVec2f(0, FBO_TIME_HEIGHT); y_max_Screen = FBO_TIME_HEIGHT; }
			else if(DataType == 4)	{ _translate = ofVec2f(0, FBO_TIME_HEIGHT); y_max_Screen = FBO_TIME_HEIGHT/2; }
			else					{ _translate = ofVec2f(0, FBO_TIME_HEIGHT/2); y_max_Screen = FBO_TIME_HEIGHT/2; }
			
			float y_max_Scale;
			if(DataType == 0)		y_max_Scale = Gui_Global->GraphScale_Time_Raw[zone];
			else if(DataType == 1)	y_max_Scale = Gui_Global->GraphScale_Time_Vel[zone];
			else if(DataType == 2)	y_max_Scale = Gui_Global->GraphScale_Time_Acc[zone];
			else if(DataType == 3)	y_max_Scale = Gui_Global->GraphScale_Time_Jerk[zone];
			else if(DataType == 4)	y_max_Scale = STATE_CHART::get_NumStates();
			
			ofColor col_back;
			if(DataType % 2 == 0)	col_back = col_Back0.get_col(255);
			else			col_back = col_Back1.get_col(255);
			
			if(DataType == 4)		drawFbo_Time(fbo_Time[zone][DataType], _translate, col_back, y_max_Screen, y_max_Scale, &Vboset_Time[zone][DataType], &Vboset_DetectClap);
			else if(DataType == 0)	drawFbo_Time(fbo_Time[zone][DataType], _translate, col_back, y_max_Screen, y_max_Scale, &Vboset_Time[zone][DataType], &Vboset_AdjustScale);
			else					drawFbo_Time(fbo_Time[zone][DataType], _translate, col_back, y_max_Screen, y_max_Scale, &Vboset_Time[zone][DataType]);
			
			if(DataType == 0)		drawFbo_ThreshLine(fbo_Time[zone][DataType], _translate, y_max_Screen, y_max_Scale, Gui_Global->thresh_raw_L[zone]);
			else if(DataType == 1)	drawFbo_ThreshLine(fbo_Time[zone][DataType], _translate, y_max_Screen, y_max_Scale, Gui_Global->thresh_vel_H[zone]);
			else if(DataType == 2)	drawFbo_ThreshLine(fbo_Time[zone][DataType], _translate, y_max_Screen, y_max_Scale, Gui_Global->thresh_acc_H[zone]);
			
			if(DataType == 1)		drawFbo_Hys_0cross_Line(fbo_Time[zone][DataType], _translate, y_max_Screen, y_max_Scale, Gui_Global->Hys_0cross_vel[zone]);
			else if(DataType == 2)	drawFbo_Hys_0cross_Line(fbo_Time[zone][DataType], _translate, y_max_Screen, y_max_Scale, Gui_Global->Hys_0cross_acc[zone]);
			else if(DataType == 3)	drawFbo_Hys_0cross_Line(fbo_Time[zone][DataType], _translate, y_max_Screen, y_max_Scale, Gui_Global->Hys_0cross_jerk[zone]);
			
			
			/* */
			drawFbo_CursorAndValue(fbo_Time[zone][DataType], zone, DataType);
		}
	}
	
	/********************
	********************/
	ofBackground(0);
	
	/* */
	drawFbo_toScreen(fbo_FFT, ofPoint(0, 0), fbo_FFT.getWidth(), fbo_FFT.getHeight());
	drawFbo_toScreen(fbo_FFT_Diff, ofPoint(0, FBO_FFT_HEIGHT), fbo_FFT_Diff.getWidth(), fbo_FFT_Diff.getHeight());
	
	for(int zone = 0; zone < NUM_FREQ_ZONES; zone++){
		for(int DataType = 0; DataType < NUM_TIMEDATA; DataType++){
			drawFbo_toScreen(fbo_Time[zone][DataType], FboTime_DispPos[zone][DataType], fbo_Time[zone][DataType].getWidth(), fbo_Time[zone][DataType].getHeight());
		}
	}
	
	/********************
	********************/
	draw_FrameRate();
	
	if(b_DispGui) Gui_Global->gui.draw();
}

/******************************
******************************/
void ofApp::draw_FrameRate()
{
	char buf[BUF_SIZE_S];
	sprintf(buf, "%5.1f", ofGetFrameRate());
	font[FONT_M].drawString(buf, ofGetWidth() - font[FONT_M].stringWidth(buf) - 10, 50);
}

/******************************
******************************/
void ofApp::SaveTimeData(){
	char buf[BUF_SIZE_S];
	sprintf(buf, "../../../data/Time_%d.csv", png_id);
	
	FILE* fp;
	fp = fopen(buf, "w");
	if(fp == NULL) return;
	
	fprintf(fp, ",Raw,Vel,Acc,Jerk,State,,,Raw,Vel,Acc,Jerk,State,,,Raw,Vel,Acc,Jerk,State\n");
	for(int i = 0; i < FBO_TIME_WIDTH; i++){
		for(int zone = 0; zone < NUM_FREQ_ZONES; zone++){
			fprintf(fp, "%d,", i);
			for(int DataType = 0; DataType < NUM_TIMEDATA; DataType++){
				char buf[BUF_SIZE_S];
				float ScreenHeight = FBO_TIME_HEIGHT/2;
				float GraphScale = 0;
				
				if(DataType == 0)			{ GraphScale = Gui_Global->GraphScale_Time_Raw[zone];	ScreenHeight = FBO_TIME_HEIGHT; }
				else if(DataType == 1)		{ GraphScale = Gui_Global->GraphScale_Time_Vel[zone]; }
				else if(DataType == 2)		{ GraphScale = Gui_Global->GraphScale_Time_Acc[zone]; }
				else if(DataType == 3)		{ GraphScale = Gui_Global->GraphScale_Time_Jerk[zone]; }
				else if(DataType == 4)		{ GraphScale = STATE_CHART::get_NumStates(); }
				
				ReverseFromVbo(buf, Vboset_Time[zone][DataType], i, ScreenHeight, GraphScale);
				fprintf(fp, "%s", buf);
				fprintf(fp, ",");
			}
			fprintf(fp, ",");
		}
		fprintf(fp, "\n");
	}
	
	fclose(fp);
}

/******************************
******************************/
void ofApp::drawFbo_CursorAndValue(ofFbo& fbo, int ZoneId, int DataType){
	/********************
	********************/
	int Cursor_x = mouseX + ofs_x_ReadCursor;
	
	if( (Cursor_x < 0) || (FBO_TIME_WIDTH <= Cursor_x) ) return;
	
	/********************
	********************/
	ofEnableAlphaBlending();
	ofEnableBlendMode(OF_BLENDMODE_ALPHA);
	
	ofEnableSmoothing();
	
	/********************
	********************/
	fbo.begin();
	ofPushStyle();
	ofPushMatrix();

		/********************
		********************/
		ofSetColor(255, 0, 0, 255);
		ofSetLineWidth(1);
		ofDrawLine(Cursor_x, 0, Cursor_x, FBO_TIME_HEIGHT);
		
		/********************
		********************/
		float ScreenHeight = FBO_TIME_HEIGHT/2;
		float GraphScale;
		
		if(DataType == 0)			{ GraphScale = Gui_Global->GraphScale_Time_Raw[ZoneId];	ScreenHeight = FBO_TIME_HEIGHT; }
		else if(DataType == 1)		{ GraphScale = Gui_Global->GraphScale_Time_Vel[ZoneId]; }
		else if(DataType == 2)		{ GraphScale = Gui_Global->GraphScale_Time_Acc[ZoneId]; }
		else if(DataType == 3)		{ GraphScale = Gui_Global->GraphScale_Time_Jerk[ZoneId]; }
		else if(DataType == 4)		{ GraphScale = STATE_CHART::get_NumStates(); }
		else						{ return; }
		
		char ch_Value[BUF_SIZE_S];
		ReverseFromVbo(ch_Value, Vboset_Time[ZoneId][DataType], Cursor_x, ScreenHeight, GraphScale);
		
		char buf[BUF_SIZE_S];
		sprintf(buf, "%4d:%s", Cursor_x, ch_Value);
		
		int ofs_x = 10;
		int ofs_y = 10;
		if(Cursor_x < FBO_TIME_WIDTH/2)	font[FONT_S].drawString(buf, Cursor_x + ofs_x, FBO_TIME_HEIGHT - ofs_y);
		else							font[FONT_S].drawString(buf, Cursor_x - ofs_x - font[FONT_S].stringWidth(buf), FBO_TIME_HEIGHT - ofs_y);
		
	ofPopMatrix();
	ofPopStyle();
	fbo.end();
}

/******************************
******************************/
void ofApp::drawFbo_toScreen(ofFbo& _fbo, const ofPoint& Coord_zero, const int Width, const int Height)
{
	/********************
	********************/
	ofDisableAlphaBlending();
	ofEnableSmoothing();
	
	/********************
	********************/
	ofPushStyle();
	ofPushMatrix();
		/********************
		********************/
		ofTranslate(Coord_zero);
		
		_fbo.draw(0, 0, Width, Height);
		
	ofPopMatrix();
	ofPopStyle();
}

/******************************
******************************/
void ofApp::drawFbo_FFT()
{
	/********************
	********************/
	ofEnableAlphaBlending();
	ofEnableBlendMode(OF_BLENDMODE_ALPHA);
	
	ofEnableSmoothing();
	
	/********************
	********************/
	fbo_FFT.begin();
	ofPushStyle();
	ofPushMatrix();
		/********************
		********************/
		ofTranslate(0, fbo_FFT.getHeight());
		ofScale(1, -1, 1);
		
		ofBackground(col_Back0.get_col(255));
		
		/********************
		y目盛り
		********************/
		const int num_lines = 2;
		const double y_step = fbo_FFT.getHeight()/num_lines;
		for(int i = 0; i < num_lines; i++){
			int y = int(i * y_step + 0.5);
			
			ofSetColor(col_White.get_col(100));
			ofSetLineWidth(1);
			ofDrawLine(0, y, fbo_FFT.getWidth(), y);

			/********************
			********************/
			ofSetColor(col_White.get_col(100));
			
			char buf[BUF_SIZE_S];
			
			/* High */
			// sprintf(buf, "%7.4f", Gui_Global->gui__Disp_FftGainMax_Diff/num_lines * i);
			sprintf(buf, "%e", Gui_Global->GraphScale_FFTGain_Monitor_H/num_lines * i);
			
			ofScale(1, -1, 1); // 文字が上下逆さまになってしまうので.
			float _x = GRAPH_BAR_SPACE__FFT_GAIN * Gui_Global->GraphScale_Zoom_FreqFrom;
			font[FONT_S].drawString(buf, _x, -y); // y posはマイナス
			ofScale(1, -1, 1); // 戻す.
			
			/* Low */
			sprintf(buf, "%e", Gui_Global->GraphScale_FFTGain_Monitor/num_lines * i);
			
			ofScale(1, -1, 1); // 文字が上下逆さまになってしまうので.
			font[FONT_S].drawString(buf, _x - font[FONT_S].stringWidth(buf) - 10, -y); // y posはマイナス
			ofScale(1, -1, 1); // 戻す.
		}
		
		/********************
		********************/
		ofSetColor(255);
		glPointSize(1.0);
		glLineWidth(1);
		
		Vboset_Monitor.draw(GL_LINES);
		Vboset_Linein.draw(GL_LINES);
		
	ofPopMatrix();
	ofPopStyle();
	fbo_FFT.end();
}

/******************************
******************************/
void ofApp::drawFbo_FFT_Diff()
{
	/********************
	********************/
	ofEnableAlphaBlending();
	ofEnableBlendMode(OF_BLENDMODE_ALPHA);
	
	ofEnableSmoothing();
	
	/********************
	********************/
	fbo_FFT_Diff.begin();
	ofPushStyle();
	ofPushMatrix();
		/********************
		********************/
		ofTranslate(0, fbo_FFT_Diff.getHeight()/2);
		ofScale(1, -1, 1);
		
		ofBackground(col_Back1.get_col(255));
		
		/********************
		y目盛り
		********************/
		const int num_lines = 2;
		const double y_step = fbo_FFT_Diff.getHeight()/2/num_lines;
		for(int i = 0; i < num_lines; i++){
			int y = int(i * y_step + 0.5);
			
			ofSetColor(col_White.get_col(100));
			ofSetLineWidth(1);
			ofDrawLine(0, y, fbo_FFT_Diff.getWidth(), y);
			if(y != 0)	ofDrawLine(0, -y, fbo_FFT_Diff.getWidth(), -y);

			/********************
			********************/
			ofSetColor(col_White.get_col(100));
			
			char buf[BUF_SIZE_S];
			
			/* High */
			sprintf(buf, "%e", Gui_Global->GraphScale_FFTGain_Diff_H/num_lines * i);
			
			ofScale(1, -1, 1); // 文字が上下逆さまになってしまうので.
			float _x = GRAPH_BAR_SPACE__FFT_GAIN * Gui_Global->GraphScale_Zoom_FreqFrom;
			font[FONT_S].drawString(buf, _x, -y); // y posはマイナス
			ofScale(1, -1, 1); // 戻す.
			
			/* Low */
			sprintf(buf, "%e", Gui_Global->GraphScale_FFTGain_Diff/num_lines * i);
			
			ofScale(1, -1, 1); // 文字が上下逆さまになってしまうので.
			font[FONT_S].drawString(buf, _x - font[FONT_S].stringWidth(buf) - 10, -y); // y posはマイナス
			ofScale(1, -1, 1); // 戻す.
		}
		
		/********************
		x
		********************/
		ofSetColor(col_White.get_col(100));
		for(int i = 0; i < AUDIO_BUF_SIZE/2; i++){
			if(i % 50 == 0){
				char buf[BUF_SIZE_S];
				sprintf(buf, "%d", i);
				
				ofScale(1, -1, 1); // 文字が上下逆さまになってしまうので.
				float _x = GRAPH_BAR_SPACE__FFT_GAIN * i;
				font[FONT_S].drawString(buf, _x, fbo_FFT_Diff.getHeight()/2); // y posはマイナス
				ofScale(1, -1, 1); // 戻す.
			}
		}
		
		/********************
		zone
		********************/
		for(int zone = 0; zone < NUM_FREQ_ZONES; zone++){
			/* */
			int x_from	= (int)Gui_Global->Zone_FreqFrom[zone] * GRAPH_BAR_SPACE__FFT_GAIN;
			int width	= (int)(Gui_Global->Zone_FreqWidth[zone]) * GRAPH_BAR_SPACE__FFT_GAIN;
			
			ofSetColor(col_Blue.get_col(40));
			ofDrawRectangle(x_from, -fbo_FFT_Diff.getHeight()/2, width, fbo_FFT_Diff.getHeight());
			
			/* */
			float RawLevel;
			if(Gui_Global->Zone_FreqFrom[zone] < Gui_Global->GraphScale_Zoom_FreqFrom)	RawLevel = fft_thread->getVal__TimeArray_x_GraphScale(zone, 0, FBO_TIME_WIDTH - 1, Gui_Global->GraphScale_FFTGain_Diff, FBO_FFT_HEIGHT/2, false);
			else																		RawLevel = fft_thread->getVal__TimeArray_x_GraphScale(zone, 0, FBO_TIME_WIDTH - 1, Gui_Global->GraphScale_FFTGain_Diff_H, FBO_FFT_HEIGHT/2, false);
			
			ofSetColor(col_Green.get_col(100));
			ofDrawRectangle(x_from, 0, width, RawLevel);
		}
		
		/********************
		********************/
		ofSetColor(255);
		glPointSize(1.0);
		glLineWidth(1);
		
		Vboset_Diff.draw(GL_LINES);
		
	ofPopMatrix();
	ofPopStyle();
	fbo_FFT_Diff.end();
}

/******************************
******************************/
void ofApp::drawFbo_Time(ofFbo& fbo, const ofVec2f _translate, const ofColor& col_back, float y_max_Screen, float y_max_Scale, VBO_SET* vboset_0, VBO_SET* vboset_1)
{
	/********************
	********************/
	ofEnableAlphaBlending();
	ofEnableBlendMode(OF_BLENDMODE_ALPHA);
	
	ofEnableSmoothing();
	
	/********************
	********************/
	fbo.begin();
	ofPushStyle();
	ofPushMatrix();
		/********************
		********************/
		ofTranslate(_translate);
		ofScale(1, -1, 1);
		
		ofBackground(col_back);
		
		/********************
		y目盛り
		********************/
		const int num_lines = 2;
		const double y_step = y_max_Screen/num_lines;
		for(int i = 0; i < num_lines; i++){
			int y = int(i * y_step + 0.5);
			
			ofSetColor(col_White.get_col(100));
			ofSetLineWidth(1);
			ofDrawLine(0, y, fbo.getWidth(), y);

			/********************
			********************/
			ofSetColor(col_White.get_col(100));
			
			char buf[BUF_SIZE_S];
			sprintf(buf, "%e", y_max_Scale/num_lines * i);
			
			ofScale(1, -1, 1); // 文字が上下逆さまになってしまうので.
			float _x = fbo.getWidth() - font[FONT_S].stringWidth(buf);
			font[FONT_S].drawString(buf, _x, -y); // y posはマイナス
			ofScale(1, -1, 1); // 戻す.
		}
		
		/********************
		********************/
		ofSetColor(255);
		glPointSize(1.0);
		glLineWidth(1);
		
		vboset_0->draw(GL_LINE_STRIP);
		if(vboset_1) vboset_1->draw(GL_LINE_STRIP);
		
	ofPopMatrix();
	ofPopStyle();
	fbo.end();
}

/******************************
******************************/
void ofApp::drawFbo_ThreshLine(ofFbo& fbo, const ofVec2f _translate, float y_max_Screen, float y_max_Scale, float thresh)
{
	/********************
	********************/
	ofEnableAlphaBlending();
	ofEnableBlendMode(OF_BLENDMODE_ALPHA);
	
	ofEnableSmoothing();
	
	/********************
	********************/
	fbo.begin();
	ofPushStyle();
	ofPushMatrix();
		/********************
		********************/
		ofTranslate(_translate);
		ofScale(1, -1, 1);
		
		/********************
		********************/
		float thresh_screen = ofMap(thresh, 0, y_max_Scale, 0, y_max_Screen, false);
		
		ofSetColor(col_Yellow.get_col(40));
		ofSetLineWidth(1);
		ofDrawRectangle(0, 0, fbo.getWidth(), thresh_screen);
		// ofDrawLine(0, thresh_screen, fbo.getWidth(), thresh_screen);
		
	ofPopMatrix();
	ofPopStyle();
	fbo.end();
}

/******************************
******************************/
void ofApp::drawFbo_Hys_0cross_Line(ofFbo& fbo, const ofVec2f _translate, float y_max_Screen, float y_max_Scale, float thresh)
{
	/********************
	********************/
	ofEnableAlphaBlending();
	ofEnableBlendMode(OF_BLENDMODE_ALPHA);
	
	ofEnableSmoothing();
	
	/********************
	********************/
	fbo.begin();
	ofPushStyle();
	ofPushMatrix();
		/********************
		********************/
		ofTranslate(_translate);
		ofScale(1, -1, 1);
		
		/********************
		********************/
		float thresh_screen = ofMap(thresh, 0, y_max_Scale, 0, y_max_Screen, false);
		
		ofSetColor(col_Red.get_col(40));
		ofSetLineWidth(1);
		ofDrawRectangle(0, -thresh_screen, fbo.getWidth(), 2 * thresh_screen);
		// ofDrawLine(0, thresh_screen, fbo.getWidth(), thresh_screen);
		
	ofPopMatrix();
	ofPopStyle();
	fbo.end();
}

/******************************
******************************/
void ofApp::SaveGuiParam_toFile()
{
	/********************
	********************/
	FILE* fp = fopen("../../../data/GUI_Param.txt", "w");
	if(fp == NULL) { printf("Unable to open GUI_Param.txt\n"); fflush(stdout); return;}
	
	char buf[BUF_SIZE_S];
	
	/********************
	********************/
	for(int zone = 0; zone < NUM_FREQ_ZONES; zone++){
		/* */
		sprintf(buf, "<MaxError_0cross_vel[%d]> %e %e %e\n", zone, (float)Gui_Global->MaxError_0cross_vel[zone], 0.0, 20.0);
		fprintf(fp, "%s", buf);
		sprintf(buf, "<MaxError_0cross_acc[%d]> %e %e %e\n", zone, (float)Gui_Global->MaxError_0cross_acc[zone], 0.0, 20.0);
		fprintf(fp, "%s", buf);
		sprintf(buf, "<MaxError_0cross_jerk[%d]> %e %e %e\n", zone, (float)Gui_Global->MaxError_0cross_jerk[zone], 0.0, 20.0);
		fprintf(fp, "%s", buf);
	}
	
	/********************
	********************/
	fclose(fp);

}

/******************************
audioIn/ audioOut
	同じthreadで動いている様子。
	また、audioInとaudioOutは、同時に呼ばれることはない(多分)。
	つまり、ofAppからaccessがない限り、変数にaccessする際にlock/unlock する必要はない。
	ofApp側からaccessする時は、threadを立てて、安全にpassする仕組みが必要
******************************/
void ofApp::audioIn(ofSoundBuffer & buffer)
{
    for (int i = 0; i < buffer.getNumFrames(); i++) {
        AudioSample.Left[i] = buffer[2*i];
		AudioSample.Right[i] = buffer[2*i+1];
    }
	
	/********************
	FFT Filtering
	1 process / block.
	********************/
	fft_thread->update__Gain(AudioSample.Left, AudioSample.Right);
}  

/******************************
******************************/
void ofApp::audioOut(ofSoundBuffer & buffer)
{
	/********************
	x	:input -> output
	o	:No output.
	********************/
    for (int i = 0; i < buffer.getNumFrames(); i++) {
		buffer[2*i  ] = AudioSample.Left[i];
		buffer[2*i+1] = AudioSample.Right[i];
    }
}

//--------------------------------------------------------------
void ofApp::keyPressed(int key){
	switch(key){
		case 'c':
		{
			ofxOscMessage m;
			m.setAddress("/DetectClap");
			m.addIntArg(0); // dummy.
			for(int i = 0; i < NUM_OSC_TARGET; i++){
				Osc[i].OscSend.sendMessage(m);
			}
		}
			break;
			
		case 'd':
			b_DispGui = !b_DispGui;
			break;
			
		case 'p':
			if(!fft_thread->Is_AdjustingScale()){
				b_PauseGraph = !b_PauseGraph;
			}
			break;
		
		case 's':
			// if( !fft_thread->Is_AdjustingScale() && !b_PauseGraph ){
			if( !b_PauseGraph ){ // Re Start ok.
				fft_thread->Start_AdjustScale();
			}
			break;
			
		case ' ':
		{
			char buf[BUF_SIZE_S];
			
			sprintf(buf, "image_%d.png", png_id);
			ofSaveScreen(buf);
			printf("> %s saved\n", buf);
			
			SaveTimeData();
			
			png_id++;
		}
			break;
			
		case OF_KEY_RIGHT:
			ofs_x_ReadCursor++;
			break;
			
		case OF_KEY_LEFT:
			ofs_x_ReadCursor--;
			break;
	}
}

//--------------------------------------------------------------
void ofApp::keyReleased(int key){

}

//--------------------------------------------------------------
void ofApp::mouseMoved(int x, int y ){
	ofs_x_ReadCursor = 0;
}

//--------------------------------------------------------------
void ofApp::mouseDragged(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::mousePressed(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::mouseReleased(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::mouseEntered(int x, int y){

}

//--------------------------------------------------------------
void ofApp::mouseExited(int x, int y){

}

//--------------------------------------------------------------
void ofApp::windowResized(int w, int h){

}

//--------------------------------------------------------------
void ofApp::gotMessage(ofMessage msg){

}

//--------------------------------------------------------------
void ofApp::dragEvent(ofDragInfo dragInfo){ 

}
