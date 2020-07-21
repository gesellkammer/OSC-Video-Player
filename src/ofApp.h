#pragma once

#include "ofMain.h"
#include "ofxOsc.h"

#define PORT 30003

typedef unsigned int ui32;
typedef unsigned long ui64;

class ofApp : public ofBaseApp{

public:
    ofApp(size_t numslots_, int port_);
    void setup();
    void update();
    void draw();

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

    void calculateDrawCoords();
    void printOscApi();
    void printKeyboardShortcuts();
    bool loadMov(int slot, string const &path);
    bool loadFolder(string const &path);
    void dumpClipsInfo();

    void sendClipInfo(ui32 idx, const string &host, int port);
    void sendClipsInfo();

    vector <ofVideoPlayer> movs;
    vector<int> loaded;
    vector<float>speeds;
	vector<int> paused;
    vector<int> shouldStop;
    size_t numSlots;
    int oscPort;
    size_t currentMov;
    bool isPlaying;
    bool isScrubbing;
    bool needsUpdate;
    bool debugging;
    int draw_x0, draw_y0, draw_width, draw_height;

    ui32 oscOutPort;
    string oscOutHost;

    ofxOscReceiver oscReceiver;
    ofxOscSender oscSender;

};
