/************************************************************
************************************************************/
#pragma once

/************************************************************
************************************************************/
#include "ofMain.h"

#include "sj_common.h"
#include "th_fft.h"


/************************************************************
************************************************************/

/**************************************************
**************************************************/
struct AUDIO_SAMPLE : private Noncopyable{
	vector<float> Left;
	vector<float> Right;
	
	void resize(int size){
		/*
		Left.resize(size);
		Right.resize(size);
		*/
		Left.assign(size, 0.0);
		Right.assign(size, 0.0);
	}
};

/**************************************************
**************************************************/
struct VBO_SET : private Noncopyable{
	ofVbo Vbo;
	vector<ofVec3f> VboVerts;
	vector<ofFloatColor> VboColor;
	
	void setup(int size){
		VboVerts.assign(size, ofVec3f(0, 0, 0)); // VboVerts.resize(size);
		VboColor.resize(size);
		
		Vbo.setVertexData(&VboVerts[0], VboVerts.size(), GL_DYNAMIC_DRAW);
		Vbo.setColorData(&VboColor[0], VboColor.size(), GL_DYNAMIC_DRAW);
	}
	
	void set_singleColor(const ofColor& color){
		for(int i = 0; i < VboColor.size(); i++) { VboColor[i].set( double(color.r)/255, double(color.g)/255, double(color.b)/255, double(color.a)/255); }
	}
	
	void set_Color(int id, int NumPerId, const ofColor& color){
		for(int i = 0; i < NumPerId; i++){
			VboColor[id * NumPerId + i].set( double(color.r)/255, double(color.g)/255, double(color.b)/255, double(color.a)/255);
		}
	}
	
	void update(){
		Vbo.updateVertexData(&VboVerts[0], VboVerts.size());
		Vbo.updateColorData(&VboColor[0], VboColor.size());
	}
	
	void draw(int drawMode){
		Vbo.draw(drawMode, 0, VboVerts.size());
	}
	
	void draw(int drawMode, int total){
		if(VboVerts.size() < total) total = VboVerts.size();
		Vbo.draw(drawMode, 0, total);
	}
};

/**************************************************
**************************************************/
class MY_COLOR{
private:
	ofColor col;
	
public:
	MY_COLOR(const ofColor& _col)
	: col(_col)
	{
	}
	
	ofColor get_col(double alpha)
	{
		ofColor _col = col;
		_col.a = alpha;
		
		return _col;
	}
};



/**************************************************
**************************************************/
class ofApp : public ofBaseApp{
private:
	/****************************************
	****************************************/
	enum{
		FONT_S,
		FONT_M,
		FONT_L,
		
		NUM_FONT_SIZE,
	};
	
	/****************************************
	****************************************/
	/********************
	********************/
	MY_COLOR col_Black;
	MY_COLOR col_White;
	MY_COLOR col_Red;
	MY_COLOR col_Green;
	MY_COLOR col_Blue;
	MY_COLOR col_Yellow;
	MY_COLOR col_Orange;
	MY_COLOR col_Pink;
	MY_COLOR col_Cian;
	MY_COLOR col_Sky;
	
	MY_COLOR col_Back0;
	MY_COLOR col_Back1;
	
	/********************
	********************/
	int soundStream_Input_DeviceId;
	int soundStream_Output_DeviceId;
	ofSoundStream soundStream;
	
	AUDIO_SAMPLE AudioSample;
	
	/********************
	********************/
	bool b_AudioDeviceSearchFailed = false;
	int t_AudioDeviceSearchFailed = 0;

	/********************
	********************/
	THREAD_FFT* fft_thread;
	
	int png_id;
	
	bool b_DispGui;
	// bool b_PauseGraph;
	
	ofTrueTypeFont font[NUM_FONT_SIZE];
	
	int ofs_x_ReadCursor;
	
	/********************
	********************/
	VBO_SET Vboset_Monitor;
	VBO_SET Vboset_Linein;
	VBO_SET Vboset_Diff;
	VBO_SET Vboset_Time[NUM_FREQ_ZONES][NUM_TIMEDATA];
	VBO_SET Vboset_DetectClap;
	
	VBO_SET Vboset_AdjustScale;
	
	ofFbo fbo_FFT;
	ofFbo fbo_FFT_Diff;
	ofFbo fbo_Time[NUM_FREQ_ZONES][NUM_TIMEDATA];
	
	ofPoint FboTime_DispPos[NUM_FREQ_ZONES][NUM_TIMEDATA] = {
		{
			ofPoint(0, FBO_FFT_HEIGHT * 2 + FBO_TIME_HEIGHT * 0), // TIMEDATA__RAW
			ofPoint(0, FBO_FFT_HEIGHT * 2 + FBO_TIME_HEIGHT * 1), // TIMEDATA__VEL
			ofPoint(0, FBO_FFT_HEIGHT * 2 + FBO_TIME_HEIGHT * 2), // TIMEDATA__ACCEL
			ofPoint(0, FBO_FFT_HEIGHT * 2 + FBO_TIME_HEIGHT * 3), // TIMEDATA__JERK
			ofPoint(0, FBO_FFT_HEIGHT * 2 + FBO_TIME_HEIGHT * 4), // TIMEDATA__STATE
		},
		{
			ofPoint(FBO_TIME_WIDTH * 1 + 5/*space*/, FBO_FFT_HEIGHT * 2 + FBO_TIME_HEIGHT * 0),
			ofPoint(FBO_TIME_WIDTH * 1 + 5/*space*/, FBO_FFT_HEIGHT * 2 + FBO_TIME_HEIGHT * 1),
			ofPoint(FBO_TIME_WIDTH * 1 + 5/*space*/, FBO_FFT_HEIGHT * 2 + FBO_TIME_HEIGHT * 2),
			ofPoint(FBO_TIME_WIDTH * 1 + 5/*space*/, FBO_FFT_HEIGHT * 2 + FBO_TIME_HEIGHT * 3),
			ofPoint(FBO_TIME_WIDTH * 1 + 5/*space*/, FBO_FFT_HEIGHT * 2 + FBO_TIME_HEIGHT * 4),
		},
		{
			ofPoint(FBO_TIME_WIDTH * 2 + 10/*space*/, FBO_FFT_HEIGHT * 2 + FBO_TIME_HEIGHT * 0),
			ofPoint(FBO_TIME_WIDTH * 2 + 10/*space*/, FBO_FFT_HEIGHT * 2 + FBO_TIME_HEIGHT * 1),
			ofPoint(FBO_TIME_WIDTH * 2 + 10/*space*/, FBO_FFT_HEIGHT * 2 + FBO_TIME_HEIGHT * 2),
			ofPoint(FBO_TIME_WIDTH * 2 + 10/*space*/, FBO_FFT_HEIGHT * 2 + FBO_TIME_HEIGHT * 3),
			ofPoint(FBO_TIME_WIDTH * 2 + 10/*space*/, FBO_FFT_HEIGHT * 2 + FBO_TIME_HEIGHT * 4),
		},
	};
	
	/****************************************
	****************************************/
	void Clear_fbo(ofFbo& fbo);
	void setup_SoundStream();
	void setup_Gui();
	void Refresh_BarColor();
	void RefreshVerts();
	bool checkif_FileExist(const char* FileName);
	void drawFbo_toScreen(ofFbo& _fbo, const ofPoint& Coord_zero, const int Width, const int Height);
	void drawFbo_FFT();
	void drawFbo_FFT_Diff();
	void drawFbo_Time(ofFbo& fbo, const ofVec2f _translate, const ofColor& col_back, float y_max_Screen, float y_max_Scale, VBO_SET* vboset_0, VBO_SET* vboset_1 = NULL);
	void drawFbo_CursorAndValue(ofFbo& fbo, int ZoneId, int DataType);
	void ReverseFromVbo(char* buf, const VBO_SET& Vboset, int id, float ScreenHeight, float GraphScale);
	void draw_FrameRate();
	void SaveTimeData();
	void drawFbo_ThreshLine(ofFbo& fbo, const ofVec2f _translate, float y_max_Screen, float y_max_Scale, float thresh);
	void drawFbo_Hys_0cross_Line(ofFbo& fbo, const ofVec2f _translate, float y_max_Screen, float y_max_Scale, float thresh);
	void SaveGuiParam_toFile();
	
public:
	/****************************************
	****************************************/
	ofApp(int _soundStream_Input_DeviceId, int _soundStream_Output_DeviceId);
	~ofApp();
	
	void setup();
	void update();
	void draw();
	void exit();
	
	void audioIn(ofSoundBuffer & buffer);
	void audioOut(ofSoundBuffer & buffer);

	void keyPressed(int key);
	void keyReleased(int key);
	void mouseMoved(int x, int y );
	void mouseDragged(int x, int y, int button);
	void mousePressed(int x, int y, int button);
	void mouseReleased(int x, int y, int button);
	void mouseEntered(int x, int y);
	void mouseExited(int x, int y);
	void windowResized(int w, int h);
	void dragEvent(ofDragInfo dragInfo);
	void gotMessage(ofMessage msg);
};
