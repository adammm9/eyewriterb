#include "trackingManager.h"
//--------------------------------------------------------------
trackingManager::trackingManager() : overlayImage(NULL)
{
}

//--------------------------------------------------------------
trackingManager::~trackingManager(){
}

//--------------------------------------------------------------
void trackingManager::setup(){

	IM.setup();
	setupGui();

	//--- set up tracking
	printf("VideoWidth, VideoHeight = %d, %d \n", IM.width, IM.height);
	tracker.setup(IM.width, IM.height);

	bFoundEye = false;
	eyePoint.set(0,0,0);

	bOriginalPositon = false;
	originalPositionB.allocate(IM.width, IM.height, OF_IMAGE_GRAYSCALE);
	originalPositionD.allocate(IM.width, IM.height, OF_IMAGE_GRAYSCALE);
}

//--------------------------------------------------------------
void trackingManager::update(){
	//--- update video/camera input
	IM.update();

	//--- eye tracking (on new frames)
	if (IM.bIsFrameNew){									// check new frame.
		trackEyes();
	}

	glintPupilVector = tracker.getVectorGlintToPupil(GLINT_BOTTOM_LEFT);

	// to make trail
	//	currentdrawPoint.x = currentdrawPoint.x * 0.80 + glintPupilVector.x * 0.20;
	//	currentdrawPoint.y = currentdrawPoint.y * 0.80 + glintPupilVector.y * 0.20;

	ofPoint	tempPoint(glintPupilVector.x, glintPupilVector.y);
	trail.push_back(tempPoint);
	if (trail.size() > 200) trail.erase(trail.begin());

	//--- gui
	panel.update();
	updateGui();

}

//--------------------------------------------------------------
ofPoint	trackingManager::getEyePoint(){
	return eyePoint;
}

//--------------------------------------------------------------
bool trackingManager::bGotAnEyeThisFrame(){
	return bFoundEye;
}

//--------------------------------------------------------------
void trackingManager::trackEyes(){
	tracker.update(*IM.grayImage);

	bFoundEye	= tracker.bFoundEye;
	eyePoint	= tracker.getVectorGlintToPupil(GLINT_BOTTOM_LEFT);					// TODO: CHECK here.

}

//--------------------------------------------------------------
void trackingManager::videoSettings(){

	// TODO: fix this!! [zach]
	//if( !bUseVideoFiles ) ((ofVideoGrabber *)videoSource)->videoSettings();
}

//--------------------------------------------------------------
void trackingManager::setOriginalPosition(){

	if ((tracker.bIsBrightEye && IM.fcount == 0) || (!tracker.bIsBrightEye && IM.fcount ==1) ||
		(IM.grabberType == INPUT_OFXLIBDC && IM.mode == INPUT_LIVE_VIDEO)) {		//Bright Eye => Left, Dark Eye =>Right
		originalPositionB.setFromPixels(IM.grayEvenImage.getPixels(), IM.width, IM.height, OF_IMAGE_GRAYSCALE, true);
		originalPositionD.setFromPixels(IM.grayOddImage.getPixels(), IM.width, IM.height, OF_IMAGE_GRAYSCALE, true);
	} else {
		originalPositionD.setFromPixels(IM.grayEvenImage.getPixels(), IM.width, IM.height, OF_IMAGE_GRAYSCALE, true);
		originalPositionB.setFromPixels(IM.grayOddImage.getPixels(), IM.width, IM.height, OF_IMAGE_GRAYSCALE, true);
	}

	bOriginalPositon = true;
}

//--------------------------------------------------------------
void trackingManager::draw(){

	if (!bFocusScreenMode){

		// Draw Input
		ofSetColor(255,255,255);
		drawInput(0, 0, IM.width/4, IM.height/4, IM.width/4, 0, IM.width/4, IM.height/4);

		// Draw EyeFinder
		tracker.eFinder.draw(0, IM.height/4+30, IM.width/2, IM.height/2);

		// Draw Pupil Finder
		tracker.thresCal.drawPupilImageWithScanLine(IM.width/2 + 20, 0, tracker.pFinder.imgBeforeThreshold.width, tracker.pFinder.imgBeforeThreshold.height, tracker.pFinder.imgBeforeThreshold);
		tracker.pFinder.draw(IM.width/2 + 20, tracker.pFinder.imgBeforeThreshold.height + 30);

		// Draw Glint Finder
		tracker.gFinder.draw(IM.width/2 + 20, tracker.pFinder.imgBeforeThreshold.height*2 + 60);
		tracker.gFinder.checkBrightEye.draw(IM.width/2 + tracker.pFinder.imgBeforeThreshold.width + 40, 255 * 2 + 60);
		tracker.gFinder.contourFinderBright.draw(IM.width/2 + tracker.pFinder.imgBeforeThreshold.width + 40, 255 * 2 + 60);

		// Draw BrightEye, DarkEye
		ofSetColor(255,255,255);
		tracker.brightEyeImg.draw(0, IM.height/4 + IM.height/2 + 60);
		tracker.darkEyeImg.draw(tracker.targetRect.width, IM.height/4 + IM.height/2 + 60);

		// Draw auto threshold Line for bright/dark eye.
		tracker.briDarkFinder.drawAutoThresholdBrightnessGraph(0, IM.height/4 + IM.height/2 + 60);

		// Draw brightness graph
		int	tempX = IM.width/2 + tracker.pFinder.imgBeforeThreshold.width + 40;
		tracker.thresCal.drawBrightnessScanGraph(tempX, 0, tracker.pFinder.imgBeforeThreshold, false, tracker.threshold_p, tracker.threshold_g, "BrightnessScan/Horizontal");
		tracker.thresCal.drawBrightnessScanGraph(tempX, 255 + 30, tracker.pFinder.imgBeforeThreshold, true, tracker.threshold_p, tracker.threshold_g, "BrightnessScan/Vertical");

		// Draw warpedImg
		if(tracker.bUseHomography && tracker.gFinder.bFourGlints){
			tracker.homographyCal.draw(IM.width/2 + tracker.pFinder.imgBeforeThreshold.width + 40, 255 * 2 + 60, 88*4, 64*4);
		}

		// Draw trail circles.
		if (bDrawRawInput) drawRawInput(ofGetWidth()/2, ofGetHeight()/2, 20);

	} else {
		drawInput(0, 0, IM.width, IM.height, IM.width, 0, IM.width, IM.height);
	}

	panel.draw();

	// this is temporary.

}

//--------------------------------------------------------------
void trackingManager::drawInput(int xBright, int yBright, int wBright, int hBright, int xDark, int yDark, int wDark, int hDark){

	if (evenBright()) {		//Bright Eye => Left, Dark Eye =>Right
		IM.drawEvenFrame(xBright, yBright, wBright, hBright);
		IM.drawOddFrame(xDark, yDark, wDark, hDark);
	} else {
		IM.drawOddFrame(xBright, yBright, wBright, hBright);
		IM.drawEvenFrame(xDark, yDark, wDark, hDark);
	}

	ofEnableAlphaBlending();
	if (bOriginalPositon){
		ofSetColor(255, 255, 255, 100);
		originalPositionB.draw(xBright, yBright, wBright, hBright);
		originalPositionD.draw(xDark, yDark, wDark, hDark);
	}
	ofDisableAlphaBlending();

	ofDrawBitmapString("Input_Bright", xBright+1, yBright+hBright + 12);			// can't display letters, if x is 0. Report the BUG.
	ofDrawBitmapString("Input_Dark", xDark+1, yDark+hDark + 12);						// can't display letters, if x is 0. Report the BUG.
}

//--------------------------------------------------------------
void trackingManager::drawRawInput(int offsetX, int offsetY, float scale){

	ofEnableAlphaBlending();

	// Draw 100 red circles.
	ofSetColor(255,0,0,40);
	for (int i = 0; i < trail.size() - 1; i++) {
		ofCircle(trail[i].x * scale + offsetX, trail[i].y * scale + offsetY, 5);
	}

	ofSetColor(255,255,0);
	ofNoFill();
	ofCircle(trail[trail.size()-1].x*20 + ofGetWidth()/2, ofGetHeight()/2 + trail[trail.size()-1].y*20, 5);

	ofDisableAlphaBlending();
}

bool trackingManager::evenBright()
{
    return ((tracker.bIsBrightEye && IM.fcount == 0) || (!tracker.bIsBrightEye && IM.fcount ==1) ||
		(IM.grabberType == INPUT_OFXLIBDC && IM.mode == INPUT_LIVE_VIDEO));		//Bright Eye => Left, Dark Eye =>Right
}

/** Stitch together grayEvenImage and grayOddImage and return result */
IplImage* trackingManager::getOverlayImage()
{
    IplImage *grayEvenImage = IM.grayEvenImage.getCvImage();
    IplImage *grayOddImage = IM.grayOddImage.getCvImage();
    assert(grayEvenImage->depth == grayOddImage->depth);
    assert(grayEvenImage->nChannels == grayOddImage->nChannels);
    if(grayEvenImage->width == 0 || grayEvenImage->height == 0) {
        ofLog(OF_LOG_WARNING, "getOverlayImage() failed. grayEvenImage->width or grayEvenImage->height = 0");
        return NULL;
    }
    if(grayOddImage->width == 0 || grayOddImage->height == 0) {
        ofLog(OF_LOG_WARNING, "getOverlayImage() failed. grayOddImage->width or grayOddImage->height = 0");
        return NULL;
    }

    ///////////////////////////////////////////////////////////////////
    // Release IplImage if sizes changed for some reason
    ///////////////////////////////////////////////////////////////////
    if(overlayImage != NULL &&
            (overlayImage->width != (grayEvenImage->width + grayOddImage->width) ||
            overlayImage->height != max(grayEvenImage->height, grayOddImage->height))) {
        cvReleaseImage(&overlayImage);
        overlayImage = NULL;
    }
    ///////////////////////////////////////////////////////////////////
    // Create IplImage if doesn't exist
    ///////////////////////////////////////////////////////////////////
    if(overlayImage == NULL) {
        overlayImage = cvCreateImage(cvSize(grayEvenImage->width + grayOddImage->width,
                                            max(grayEvenImage->height, grayOddImage->height)),
                                     grayEvenImage->depth, grayEvenImage->nChannels);
    }

    ///////////////////////////////////////////////////////////////////
    // Copy grayEvenImage and grayOddImage contents into
    // overlayImage which will store both side by side.
    // Bright eye goes on left side of image.
    ///////////////////////////////////////////////////////////////////
    if(evenBright()) {
        //Even first, then odd
        cvSetImageROI(overlayImage, cvRect(0, 0, grayEvenImage->width, grayEvenImage->height));
        cvCopy(grayEvenImage, overlayImage);
        cvSetImageROI(overlayImage, cvRect(grayEvenImage->width, 0, grayOddImage->width, grayOddImage->height));
        cvCopy(grayOddImage, overlayImage);
    } else {
        //Odd first, then even
        cvSetImageROI(overlayImage, cvRect(0, 0, grayOddImage->width, grayOddImage->height));
        cvCopy(grayOddImage, overlayImage);
        cvSetImageROI(overlayImage, cvRect(grayOddImage->width, 0, grayEvenImage->width, grayEvenImage->height));
        cvCopy(grayEvenImage, overlayImage);
    }
    cvResetImageROI(overlayImage);
    return overlayImage;
}

//--------------------------------------------------------------
void trackingManager::mouseDragged(int x, int y, int button){
	panel.mouseDragged(x, y, button);
}
//--------------------------------------------------------------
void trackingManager::mousePressed(int x, int y, int button){
	panel.mousePressed(x, y, button);
	trail.clear();
}
//--------------------------------------------------------------
void trackingManager::mouseReleased(){
	panel.mouseReleased();
}
