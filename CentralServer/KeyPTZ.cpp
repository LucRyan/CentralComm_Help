
#include "KeyPTZ.h"

ArVCC4* G_PTZHandler;
/*
Commands:
_________________

UP,DOWN     -- tilt up/down by 1 degree
LEFT,RIGHT  -- pan left/right by 1 degree
X,C         -- zoom in/out by 10 units (80 total)
I           -- initialize PTU to default settings
>,<         -- increase/decrease the positional increment by one 1 degree
+,-         -- increase/decrease the slew by 5 degrees/sec
Z           -- move pan and tilt axes to zero
H           -- Halt all motion
S           -- Status of camera position and variable values
P           -- Power on/off the camera
ESC         -- Exit program
*/


/*
  Constructor, sets the robot pointer, and some initial values, also note the
  use of constructor chaining on myPTU and myDriveCB.
*/

KeyPTU::KeyPTU(ArRobot *robot) :
  myUpCB(this, &KeyPTU::up),
  myDownCB(this, &KeyPTU::down),
  myLeftCB(this, &KeyPTU::left),
  myRightCB(this, &KeyPTU::right),
  myPlusCB(this, &KeyPTU::plus),
  myMinusCB(this, &KeyPTU::minus),
  myGreaterCB(this, &KeyPTU::greater),
  myLessCB(this, &KeyPTU::less),
  myQuestionCB(this, &KeyPTU::question),
  mySCB(this, &KeyPTU::status),
  myECB(this, &KeyPTU::exercise),
  myACB(this, &KeyPTU::autoupdate),
  myCCB(this, &KeyPTU::c),
  myHCB(this, &KeyPTU::h),
  myICB(this, &KeyPTU::i),
  myPCB(this, &KeyPTU::p),
  myXCB(this, &KeyPTU::x),
  myZCB(this, &KeyPTU::z),
  myRCB(this, &KeyPTU::r),

  myPTU(robot,false,ArVCC4::COMM_BIDIRECTIONAL,true,true,ArVCC4::CAMERA_C50I),
  myDriveCB(this, &KeyPTU::drive)
{

  // set the robot pointer and add the KeyPTU as user task
  ArKeyHandler *keyHandler;
  myRobot = robot;
  myRobot->addSensorInterpTask("KeyPTU", 50, &myDriveCB);

  myPTU.setLEDControlMode(2);
  myExerciseTime.setToNow();
  myExercise = false;
  G_PTZHandler = &myPTU;
  myPTU.enableIRFilterMode();
  myPTU.enableIRLEDs();
	
//  SETPORT Uncomment the following to run the camera off
//  of the computer's serial port, rather than the microcontroller

// uncomment below here
/*
#ifdef WIN32
  myCon.setPort("COM2");
#else
  myCon.setPort("/dev/ttyS0");
#endif
  myPTU.setDeviceConnection(&myCon);
*/
// to here

  // or use this next line to set the aux port 
 //myPTU.setAuxPort(2);


  if ((keyHandler = Aria::getKeyHandler()) == NULL)
  {
    keyHandler = new ArKeyHandler;
    Aria::setKeyHandler(keyHandler);
    myRobot->attachKeyHandler(keyHandler);
  }

  if (!keyHandler->addKeyHandler(ArKeyHandler::UP, &myUpCB))
    ArLog::log(ArLog::Terse, "The key handler already has a key for up, keydrive will not work correctly.");
  if (!keyHandler->addKeyHandler(ArKeyHandler::DOWN, &myDownCB))
    ArLog::log(ArLog::Terse, "The key handler already has a key for down, keydrive will not work correctly.");
  if (!keyHandler->addKeyHandler(ArKeyHandler::LEFT, &myLeftCB))
    ArLog::log(ArLog::Terse,  
"The key handler already has a KeyPTUkey for left, keydrive will not work correctly.");
  if (!keyHandler->addKeyHandler(ArKeyHandler::RIGHT, &myRightCB))
    ArLog::log(ArLog::Terse,  
"The key handler already has a key for right, keydrive will not work correctly.");

  if (!keyHandler->addKeyHandler('+', &myPlusCB))
    ArLog::log(ArLog::Terse,
"The key handler already has a key for '+', keydrive will not work correctly.");
  if (!keyHandler->addKeyHandler('-', &myMinusCB))
    ArLog::log(ArLog::Terse,
"The key handler already has a key for '-', keydrive will not work correctly.");
  if (!keyHandler->addKeyHandler('>', &myGreaterCB))
    ArLog::log(ArLog::Terse,
"The key handler already has a key for '>', keydrive will not work correctly.");
  if (!keyHandler->addKeyHandler('<', &myLessCB))
    ArLog::log(ArLog::Terse,
"The key handler already has a key for '<', keydrive will not work correctly.");
  if (!keyHandler->addKeyHandler('?', &myQuestionCB))
    ArLog::log(ArLog::Terse,
"The key handler already has a key for '?', keydrive will not work correctly.");

  if (!keyHandler->addKeyHandler('c', &myCCB))
    ArLog::log(ArLog::Terse,
"The key handler already has a key for 'C', keydrive will not work correctly.");
  if (!keyHandler->addKeyHandler('h', &myHCB))
    ArLog::log(ArLog::Terse,
"The key handler already has a key for 'H', keydrive will not work correctly.");
  if (!keyHandler->addKeyHandler('i', &myICB))
    ArLog::log(ArLog::Terse,
"The key handler already has a key for 'I', keydrive will not work correctly.");
  if (!keyHandler->addKeyHandler('p', &myPCB))
    ArLog::log(ArLog::Terse,
"The key handler already has a key for 'P', keydrive will not work correctly.");
  if (!keyHandler->addKeyHandler('r', &myRCB))
    ArLog::log(ArLog::Terse,
"The key handler already has a key for 'R', keydrive will not work correctly.");

  if (!keyHandler->addKeyHandler('s', &mySCB))
    ArLog::log(ArLog::Terse,
"The key handler already has a key for 'S', keydrive will not work correctly.");
  if (!keyHandler->addKeyHandler('x', &myXCB))
    ArLog::log(ArLog::Terse,
"The key handler already has a key for 'X', keydrive will not work correctly.");
  if (!keyHandler->addKeyHandler('z', &myZCB))
    ArLog::log(ArLog::Terse,
"The key handler already has a key for 'Z', keydrive will not work correctly.");
  if (!keyHandler->addKeyHandler('a', &myACB))
    ArLog::log(ArLog::Terse, "The key handler already has a key for 'A', keydrive will not work correctly.");
  if (!keyHandler->addKeyHandler('e', &myECB))
    ArLog::log(ArLog::Terse, "The key handler already has a key for 'E', keydrive will not work correctly.");



  // initialize some variables

  myPTUInitRequested = false;
  myPosIncrement = 1;
  mySlewIncrement = 5;
  myZoomIncrement = 50;
	myPTU.tiltRel(-3);

}

void KeyPTU::autoupdate(void)
{
  if (myPTU.getAutoUpdate())
    myPTU.disableAutoUpdate();
  else
    myPTU.enableAutoUpdate();
}

void KeyPTU::right(void)
{
  myPTU.panRel(myPosIncrement);
}

void KeyPTU::left(void)
{
  myPTU.panRel(-myPosIncrement);
}

void KeyPTU::up(void)
{
  myPTU.tiltRel(myPosIncrement);
}

void KeyPTU::down(void)
{
  myPTU.tiltRel(-myPosIncrement);
}

void KeyPTU::x(void)
{
  myPTU.zoom(myPTU.getZoom() + myZoomIncrement);
}

void KeyPTU::c(void)
{
  myPTU.zoom(myPTU.getZoom() - myZoomIncrement);
}

void KeyPTU::i(void)
{
  myPTU.init();
}

void KeyPTU::plus(void)
{
  myPTU.panSlew(myPTU.getPanSlew() + mySlewIncrement);
  myPTU.tiltSlew(myPTU.getTiltSlew() + mySlewIncrement);

  status();
}

void KeyPTU::minus(void)
{
  myPTU.panSlew(myPTU.getPanSlew() - mySlewIncrement);
  myPTU.tiltSlew(myPTU.getTiltSlew() - mySlewIncrement);

  status();
}

void KeyPTU::greater(void)
{
  myPosIncrement += 1;
  
  if (myPosIncrement > myPTU.getMaxPosPan())			//Use pan range as reference for largest allowable positional increment
    myPosIncrement = myPTU.getMaxPosPan();

  status();
}

void KeyPTU::less(void)
{
  myPosIncrement -= 1;

  if (myPosIncrement < 0)
    myPosIncrement = 0;

  status();
}


void KeyPTU::z(void)
{
  myPTU.panTilt(0,0);
  myPTU.zoom(0);
  status();
}


void KeyPTU::r(void)
{
  myPTU.reset();
		ArUtil::sleep(200);
	G_PTZHandler->tiltRel(-10);
}


void KeyPTU::question(void)
{
  ArLog::log(ArLog::Normal, "\r\nCommands:\r\n_________________\r\n");
  ArLog::log(ArLog::Normal, "UP,DOWN     -- tilt up/down by 1 increment");
  ArLog::log(ArLog::Normal, "LEFT,RIGHT  -- pan left/right by 1 increment");
  ArLog::log(ArLog::Normal, "X,C         -- zoom in/out by 50 units (2140 max)");
  ArLog::log(ArLog::Normal, "I           -- initialize PTU to default settings");
  ArLog::log(ArLog::Normal, ">,<         -- increase/decrease the positional increment by 1 degree");
  ArLog::log(ArLog::Normal, "+,-         -- increase/decrease the slew by 5 degrees/sec");
  ArLog::log(ArLog::Normal, "Z           -- move pan and tilt axes to zero");
  ArLog::log(ArLog::Normal, "E		 -- toggle exercise mode");
  ArLog::log(ArLog::Normal, "A		 -- toggle autoupdate mode");
  ArLog::log(ArLog::Normal, "H           -- Halt all motion");
  ArLog::log(ArLog::Normal, "S           -- Status of camera position and variable values");
  ArLog::log(ArLog::Normal, "P           -- Power on/off the camera");
  ArLog::log(ArLog::Normal, "R           -- Reset the camera");
  ArLog::log(ArLog::Normal, "ESC         -- Exit program");
  ArLog::log(ArLog::Normal, "\r\n");
}

void KeyPTU::status(void)
{
  ArLog::log(ArLog::Normal, "\r\nStatus:\r\n_________________________\r\n");
  ArLog::log(ArLog::Normal, "Pan Position       =  %.0f deg", myPTU.getPan());
  ArLog::log(ArLog::Normal, "Tilt Position      =  %.0f deg", myPTU.getTilt());
  ArLog::log(ArLog::Normal, "Zoom Position      =  %d", myPTU.getZoom());
  ArLog::log(ArLog::Normal, "Pan Slew           =  %d deg/s", myPTU.getPanSlew());
  ArLog::log(ArLog::Normal, "Tilt Slew          =  %d deg/s", myPTU.getTiltSlew());
  ArLog::log(ArLog::Normal, "Position Increment =  %d deg", myPosIncrement);
  ArLog::log(ArLog::Normal, "Autoupdate         =  %d", myPTU.getAutoUpdate());
  ArLog::log(ArLog::Normal, "Exercise           =  %d", myExercise);
  if (myPTU.getPower())
    ArLog::log(ArLog::Normal, "Power is ON");
  else
    ArLog::log(ArLog::Normal, "Power is OFF");
  ArLog::log(ArLog::Normal, "\r\n");
}

void KeyPTU::h(void)
{
  myPTU.haltPanTilt();
  myPTU.haltZoom();
}

void KeyPTU::p(void)
{
  if (myPTU.getPower())
    myPTU.power(0);
  else
    myPTU.power(1);

  status();
}

// the important function
void KeyPTU::drive(void)
{
  // if the PTU isn't initialized, initialize it here... it has to be 
  // done here instead of above because it needs to be done when the 
  // robot is connected
  if (!myPTUInitRequested && !myPTU.isInitted() && myRobot->isConnected())
  {
    printf("\nWaiting for Camera to Initialize\n");
    myAbsolute = true;
    myPTUInitRequested = true;
    myPTU.init();
  }

  // if the camera hasn't initialized yet, then just return
  if (myPTUInitRequested && !myPTU.isInitted())
  {
    //return;
  }

  if (myPTUInitRequested && myPTU.isInitted())
  {
    myPTUInitRequested = false;
    myPanSlew = myPTU.getPanSlew();
    myTiltSlew = myPTU.getTiltSlew();
    printf("Done.\n");
    question();
  }

  if (myExerciseTime.secSince() > 5 && myExercise)
  {
    int pan,tilt;

    if (ArMath::random()%2)
      pan = ArMath::random()%((int)myPTU.getMaxPosPan());
    else
      pan = -ArMath::random()%((int)myPTU.getMaxNegPan());

    if (ArMath::random()%2)
      tilt = ArMath::random()%((int)myPTU.getMaxPosTilt());
    else
      tilt = -ArMath::random()%((int)myPTU.getMaxNegTilt());

    myPTU.panTilt(pan, tilt);
    //printf("** %d\n", myRobot->getEstop());
    //printf("--> %x\n", myRobot->getFlags());
    myExerciseTime.setToNow();
  }

}