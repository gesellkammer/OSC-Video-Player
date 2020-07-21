
#include "ofApp.h"

#define LOG  ofLogVerbose()
#define INFO ofLogNotice()
#define ERR  ofLogError()
#define WARN ofLogWarning()


//--------------------------------------------------------------
ofApp::ofApp(size_t numslots_, int port_) {
    numSlots = numslots_;
    oscPort = port_;
    oscOutPort = 0;
    oscOutHost = "";

    debugging = true;
    currentMov = 0;
    isPlaying = 0;
    needsUpdate = 0;
    draw_x0 = 0;
    draw_y0 = 0;

    for (int i=0; i < numSlots; i++) {
        loaded.push_back(0);
        movs.push_back(ofVideoPlayer());
        speeds.push_back(1);
        paused.push_back(0);
        shouldStop.push_back(0);
    }
}

void ofApp::setup(){
    // numSlots = NUMSLOTS;
    draw_height = ofGetWindowHeight();
    draw_width = ofGetWindowWidth();
    oscReceiver.setup(oscPort);
    if(oscOutPort != 0) {
        oscSender.setup(oscOutHost, oscOutPort);
    }
    // oscSender.setup()
    ofBackground(0);    
    printOscApi();
}

void ofApp::printOscApi() {
    cout << "OSC port: " << oscPort << "\n";
    cout << "Number of Slots: " << numSlots << "\n";
    cout << "\nOSC messages accepted:\n\n";

    cout << "/load slot:int path:str\n";
    cout << "    * Load a video at the given slot\n\n";
    
    cout << "/play slot:int [speed:float=1] [starttime:float=0] [paused:int=0] [stopWhenFinished:int=1] \n";
    cout << "    * Play the given slot with given speed, starting at starttime (secs)\n\n";
    cout << "      paused: if 1, the playback will be paused\n";
    cout << "      stopWhenFinished: if 1, playback will stop at the end, otherwise it\n";
    cout << "        pauses at the last frame\n\n";

    cout << "/stop\n";
    cout << "    * Stop playback\n\n";

    cout << "/pause state:int\n";
    cout << "    * If state 1, pause playback, 0 resumes playback\n\n";

    cout << "/setspeed slot:int speed:float\n";
    cout << "    * Change the speed of a given slot\n";

    cout << "/scrub pos:float [slot:int=current] \n";
    cout << "    * Set the relative position 0-1\n";

    cout << "/dump\n";
    cout << "    * Dump information about loaded clips\n";

    cout << "/quit\n";
    cout << "    * Quit this application\n";
}

void ofApp::printKeyboardShortcuts() {
    cout << "Keyboard Shortcuts\n";
    cout << "==================\n\n";
    cout << "F - Toggle Fullscreen \n";
    cout << "D - Dump Information about loaded clips \n";
    cout << "Q - Quit \n";
}


//--------------------------------------------------------------
bool ofApp::loadMov(int slot, const string &path) {
    if(slot < 0 && slot >= numSlots) {
        ERR << "loadMov -- Slot out of range: " << slot << endl;
        return false;
    }
    auto idx = static_cast<size_t>(slot);
    if(loaded[idx] == 0) {
        LOG << "Closing movie idx: " << idx;
        movs[idx].close();
    }
    movs[idx].setPixelFormat(OF_PIXELS_RGB);
    movs[idx].load(path);
    movs[idx].setLoopState(OF_LOOP_NONE);
    // movs[idx].play();
    loaded[idx] = 1;
    INFO << "Loaded slot " << slot << ": " << path << endl;
    if(this->oscOutPort != 0) {
        this->sendClipInfo(idx, this->oscOutHost, this->oscOutPort);
    } else
        LOG << "not sending OSC notificacion";
    return true;
}

void ofApp::update(){

    while(oscReceiver.hasWaitingMessages()) {
        ofxOscMessage msg;
        oscReceiver.getNextMessage(msg);
        string addr = msg.getAddress();
        auto numargs = msg.getNumArgs();
        if (addr == "/scrub") {
            if(numargs == 1 || numargs == 2) {
                size_t idx = numargs == 1 ? currentMov : msg.getArgAsInt32(1);
                isPlaying = 1;
                if(idx != currentMov) {
                    if(!loaded[idx]) {
                        ERR << "/scrub: current index "<<currentMov<< "not loaded\n";
                        continue;
                    }
                    movs[currentMov].stop();
                    movs[idx].play();
                    movs[idx].setSpeed(0);
                    movs[idx].setPaused(true);
                    currentMov = idx;
                    calculateDrawCoords();
                }
                float pos = msg.getArgAsFloat(0);

                movs[idx].setPosition(pos);
            } else {
                ERR << "/scrub expected 1 or 2 args, received " << numargs << endl;
                ERR << "    /scrub relpos:float";
                continue;
            }
        }
        else if(addr == "/setpos") {
            float pos = msg.getArgAsFloat(0);
            if(loaded[currentMov]) {
                movs[currentMov].setPosition(pos);
            }
        }
        else if(addr == "/play") {
            // args: slot, speed, skip(seconds), paused, stop_when_finished
            if(numargs == 0 || numargs > 5) {
                ERR << "/play expects 1-5 arguments, got " << numargs << endl
                    << "    /play slot:int, speed:float=1, skipsecods:float=0, paused:int=0, stopWhenFinished:int=1\n";
                continue;
            }
            int slot = msg.getArgAsInt32(0);
            if(slot < 0 || slot >= numSlots) {
                ERR << "/play: slot "<<slot<< " out of range\n";
                continue;
            }
            auto newIdx = static_cast<size_t>(slot);
            if(!loaded[newIdx]) {
                ERR << "/play: slot "<<slot<< "not loaded\n";
                continue;
            }
            float speed = numargs >= 2 ? msg.getArgAsFloat(1) : 1.0;
            float skiptime = numargs >= 3 ? msg.getArgAsFloat(2) : 0;
            int pausestatus = numargs >= 4 ? msg.getArgAsInt32(3) : 0;
            int stopWhenFinished = numargs >= 5 ? msg.getArgAsInt32(4) : 1;

            if(isPlaying) {
                movs[currentMov].stop();
            }

            currentMov = newIdx;
            auto mov = &(movs[currentMov]);
            speeds[currentMov] = speed;
            paused[currentMov] = pausestatus;
            mov->play();
            mov->setSpeed(speed);
            // mov->setPaused(pausestatus);
            shouldStop[currentMov] = stopWhenFinished;
            // setPosition is between 0-1
            mov->setPosition(skiptime/mov->getDuration());
            isPlaying = 1;
            calculateDrawCoords();
            INFO << "/play - slot:" << slot << ", speed:" << speed
                 << ", skiptime:" << skiptime << ", paused: " << pausestatus
                 << ", stop_when_finished: " << stopWhenFinished << endl;
            break;
        }
        else if(addr == "/stop") {
            if(!isPlaying) {
                ERR << "Not playing!\n";
                continue;
            }
            /* Don't flag this as error, just ignore the arguments
            if(numargs != 0) {
                ERR << "/stop expects 0 arguments, got " << numargs << endl
                    << "    /stop slot:int=current \n";
            }
            */
            if(!loaded[currentMov]) {
                ERR << "/stop slot " << currentMov << " not loaded\n";
                continue;
            }
            LOG << "stopping slot: " << currentMov << "\n";
            movs[currentMov].stop();
            isPlaying = 0;
        }
        else if(addr == "/pause") {
            if(numargs != 1) {
                ERR << "/pause expects 1 arguments, got " << numargs << endl
                    << "    Syntax: /pause pauseStatus:int\n";
                continue;
            }
            int status = msg.getArgAsInt32(0);
            if(!isPlaying) {
                cout << "Clip not playing\n";
            }
            else if(!loaded[currentMov]) {
                ERR << "slot not loaded: " << currentMov << endl;
            }
            else {
                movs[currentMov].setPaused(status);
            }
        }
        else if(addr == "/load") {
            if(numargs != 2) {
                ERR << "/load expects 2 arguments, got " << numargs << endl
                    << "    /load slot:int path:string\n";
                continue;
            }
            int slot = msg.getArgAsInt32(0);
            if(slot < 0 || slot >= numSlots) {
                ERR << "/load: slot "<<slot<<" out of range\n";
                continue;
            }

            string path = msg.getArgAsString(1);
            auto ok = loadMov(slot, path);
            if(!ok) {
                ERR << "Could not load movie " << path << endl;
            } else {
                INFO << "/load - slot:" << slot << ", path:" << path << "\n";
            }
        }
        else if(addr == "/loadfolder") {
            if(numargs != 1) {
                ERR << "/loadfolder expects 1 arguments, got " << numargs << endl
                    << "    Syntax: /loadfolder path:string\n";
                continue;
            }
            // the name pattern is XXX_descr.ext, where XXX is the slot number.
            // right now we dont do anything with descr
            string path = msg.getArgAsString(0);
            bool ok = loadFolder(path);
            if(!ok) {
                ERR << "/loadfolder could not load some of the samples \n";
            }
        }
        else if(addr == "/dump") {
            /*
            if(numargs != 0) {
                ERR << "/dump expects 0 arguments, got " << numargs << endl;
                continue;
            }
            */
            this->dumpClipsInfo();
        }
        else if(addr == "/setspeed") {
            if(numargs != 1) {
                ERR << "/setspeed expects 1 arguments, got " << numargs << endl;
                continue;
            }
            size_t idx = static_cast<size_t>(this->currentMov);
            if(!loaded[idx]) {
                ERR << "/setspeed: slot "<<idx<<" not loaded\n";
                return;
            }
            float speed = msg.getArgAsFloat(0);
            movs[idx].setSpeed(speed);
        }
        else {
            ERR << "Message not recognized: " << addr << endl;
        }
    }
    if (isPlaying == 1 && loaded[currentMov]) {
        if(needsUpdate == 1) {
            auto mov = &(movs[currentMov]);
            mov->setSpeed(speeds[currentMov]);
            mov->setPaused(paused[currentMov]);
            needsUpdate = 0;
        }
        if(movs[currentMov].getIsMovieDone()) {
            if( shouldStop[currentMov] ) {
                movs[currentMov].stop();
                isPlaying = 0;
            } else {
                movs[currentMov].setPaused(true);
            }
        }
        movs[currentMov].update();
    }
}

void ofApp::calculateDrawCoords() {
    if(currentMov >= 0) {
        auto mov = &(movs[currentMov]);
        float wr = ofGetWindowWidth() / mov->getWidth();
        float hr = ofGetWindowHeight() / mov->getHeight();
        float draw_h, draw_w;
        int x0, y0;
        if (hr < wr) {
            draw_h = ofGetWindowHeight();
            draw_w = draw_h * (mov->getWidth() / mov->getHeight());
            x0 = (ofGetWindowWidth() - draw_w) / 2.0;
            y0 = 0;
        } else {
            draw_w = ofGetWindowWidth();
            draw_h = draw_w * (mov->getHeight() / mov->getWidth());
            x0 = 0;
            y0 = (ofGetWindowHeight() - draw_h) / 2.0;
        }
        draw_x0 = x0;
        draw_y0 = y0;
        draw_height = int(draw_h);
        draw_width = int(draw_w);
    } else {
        draw_x0 = 0;
        draw_y0 = 0;
        draw_height = ofGetWindowHeight();
        draw_width = ofGetWindowWidth();
    }
}

bool ofApp::loadFolder(const string &path) {
    // the name pattern is XXX_descr.ext, where XXX is the slot number.
    // right now we dont do anything with descr
    ofDirectory dir(path);
    dir.allowExt("mp4");
    dir.allowExt("mkv");
    dir.allowExt("mpg");
    dir.allowExt("avi");
    dir.allowExt("ogv");
    dir.allowExt("m4v");
    dir.allowExt("mov");
    //populate the directory object
    dir.listDir();

    for(size_t i = 0; i < dir.size(); i++){
        string filename = dir.getName(i);
        LOG << "loadFolder: loading " << filename << endl;
        auto delim = filename.find("_");
        // dont accept extremely long names
        if (delim >= 100) {
            ERR << "/loadfolder: filename should have the format XXX_descr.ext\n"
                << "   filename: " << filename << endl;
            return false;
        } else {
            int slot = std::stoi(filename.substr(0, delim));
            if(slot < 0 || slot >= numSlots) {
                ERR << "/loadfolder: slot out of range: " << slot << endl
                    << "    filename: " << filename << endl;
            }
            size_t idx = static_cast<size_t>(slot);
            if(loaded[idx]) {
                INFO << "/loadfolder: loading a clip in an already used slot\n"
                     << "    Slot: " << slot << endl
                     << "    New clip: " << filename << endl
                     << "    Previous clip: " << movs[idx].getMoviePath() << endl;
            }
            LOG << "loading slot: " << slot << ", path: " << dir.getPath(i);
            auto ok = this->loadMov(slot, dir.getPath(i));
            if(!ok) {
                ERR << "Could not load clip \n";
                return false;
            }
        }
    }
    return true;
}

//--------------------------------------------------------------
void ofApp::draw(){
    if (isPlaying==1 && loaded[currentMov]) {
        movs[currentMov].draw(draw_x0, draw_y0, draw_width, draw_height);
    }
}

void ofApp::dumpClipsInfo() {
    cout << "Loaded Clips: \n";
    for(size_t i=0; i<numSlots; i++) {
        if(!loaded[i])
            continue;

        cout << "  * slot:" << i
             << ", path: " << movs[i].getMoviePath()
             << ", dur:" << movs[i].getDuration()
             << "\n";
    }
}


void ofApp::sendClipInfo(ui32 idx, const string &host, int port) {
    LOG << "sendClipInfo slot: " << idx << ", " << host << ":" << port;
    oscSender.setup(host, port);
    ofxOscMessage msg;
    msg.setAddress("/clipinfo");
    msg.addIntArg(idx);
    msg.addStringArg(movs[idx].getMoviePath());
    msg.addFloatArg(movs[idx].getDuration());
    oscSender.sendMessage(msg);
}

void ofApp::sendClipsInfo() {
    if(this->oscOutPort == 0) {
        ERR << "sendClipsInfo: out osc port not set \n";
        return;
    }
    for(int i=0; i<numSlots; i++) {
        if(!loaded[i])
            continue;
        sendClipInfo(i, this->oscOutHost, this->oscOutPort);
    }
}

//--------------------------------------------------------------
void ofApp::keyPressed(int key) {
    switch(key) {
    case 'f':
        ofToggleFullscreen();
        break;
    case 'q':
        ofExit();
        break;
    case 'd':
        this->dumpClipsInfo();
        break;
    default:
        LOG << "Unknown key: " << key;
    }

}

//--------------------------------------------------------------
void ofApp::keyReleased(int key) {

}

//--------------------------------------------------------------
void ofApp::mouseMoved(int x, int y ){

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
    calculateDrawCoords();
}

//--------------------------------------------------------------
void ofApp::gotMessage(ofMessage msg){

}

//--------------------------------------------------------------
void ofApp::dragEvent(ofDragInfo dragInfo){

}
