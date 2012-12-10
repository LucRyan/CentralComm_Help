#include "Aria.h"
#include "ArNetworking.h"
#include "Arnl.h"
#include "ArLocalizationTask.h"


//#include "opencv2/highgui/highgui.hpp"
//#include "opencv2/features2d/features2d.hpp"
//#include "opencv2/imgproc/imgproc.hpp"


#include "VCCHandler.h"
#include "VideoServer.h"
#include "MotionsServer.h"


#include <iostream>
// #include <sstream>
#include "VideoServer.h"
#include <opencv2/opencv.hpp>

//#include <stdio.h>
//#include <stdlib.h>





using namespace std;
using namespace cv;

// ArRobot robot(NULL, false);

//------------------------Global Variables----------------------------------------
ArPathPlanningTask* G_PathPlanning;
ArRobot robot;
ArSick sick;
// ArAnalogGyro *G_gyro;

//-------------------------video stream feedback-----------------------------------------


int main(int argc, char **argv)
{

//----------------------initialized robot server------------------------------------------
  Aria::init();
  Arnl::init();
  
  ArServerBase server;
	
//-----------------------------------------------------------------------------------
	VCCHandler ptz(&robot); 		//create keyboard for control vcc50i

	
	G_PTZHandler->reset();
	ArUtil::sleep(300);
	G_PTZHandler->panSlew(30);
//-----------------------------------------------------------------------------------

  argc = 2 ;

  argv[0] = "-map";
  argv[1] = "map20121111.map";
  
  // Parse the command line arguments.
  ArArgumentParser parser(&argc, argv);

  // Set up our simpleConnector
  ArSimpleConnector simpleConnector(&parser);

  // Set up our simpleOpener
  ArServerSimpleOpener simpleOpener(&parser);
  

//*******
  // Set up our client for the central server
//   ArClientSwitchManager clientSwitch(&server, &parser);
//************
  
  // Load default arguments for this computer (from /etc/Aria.args, environment
  // variables, and other places)
//   parser.loadDefaultArguments();

  // set up a gyro
  ArAnalogGyro gyro(&robot);
  //gyro.activate();
  // Parse arguments for the simple connector.
//   if (!Aria::parseArgs() || !parser.checkHelpAndWarnUnparsed())
//   {
//     ArLog::log(ArLog::Normal, "\nUsage: %s -map mapfilename\n", argv[0]);
//     Aria::logOptions();
//     Aria::exit(1);
//   }


  // The laser object, will be used if we have one


  // Add the laser to the robot
  robot.addRangeDevice(&sick);

  // Sonar, must be added to the robot, used by teleoperation and wander to
  // detect obstacles, and for localization if SONARNL
  ArSonarDevice sonarDev;

  // Add the sonar to the robot
  robot.addRangeDevice(&sonarDev);
  
  // Set up where we'll look for files
  char fileDir[1024];
  ArUtil::addDirectories(fileDir, sizeof(fileDir), "./", "maps");
  ArLog::log(ArLog::Normal, "Installation directory is: %s\nMaps directory is: %s\n", Aria::getDirectory(), fileDir);
  
  // Set up the map, this will look for files in the examples
  // directory (unless the file name starts with a /, \, or .
  // You can take out the 'fileDir' argument to look in the current directory
  // instead
  
  ArMap arMap(fileDir);
  // set it up to ignore empty file names (otherwise the parseFile
  // on the config will fail)
  arMap.setIgnoreEmptyFileName(true);
  
//********************************
//Localization
//********************************
  

  ArLocalizationManager locManager(&robot, &arMap);
  ArLog::log(ArLog::Normal, "Creating laser localization task");
  ArLocalizationTask locTask (&robot, &sick, &arMap);
  locManager.addLocalizationTask(&locTask);

  
//*******************************
//Path planning
//*******************************
  
  // Make the path task planning task
  ArPathPlanningTask pathTask(&robot, &sick, &sonarDev, &arMap);
  G_PathPlanning = &pathTask;
  // Set up things so data can be logged (only do it with the laser
  // since it can overrun a 9600 serial connection which the sonar is
  // more likely to have)
  ArDataLogger dataLogger(&robot);
  dataLogger.addToConfig(Aria::getConfig());

  
  
  
  // add our logging to the config
//   ArLog::addToConfig(Aria::getConfig());

  // First open the server 
  if (!simpleOpener.open(&server, fileDir, 240))
  {
    if (simpleOpener.wasUserFileBad())
      ArLog::log(ArLog::Normal, "Bad user file");
    else
      ArLog::log(ArLog::Normal, "Could not open server port");
    exit(2);
  }

  // Connect the robot
  if (!simpleConnector.connectRobot(&robot))
  {
    ArLog::log(ArLog::Normal, "Could not connect to robot... exiting");
    Aria::exit(3);
  }

  
//-----------------------------------------------
//**************************
  // Set up a class that'll put the movement and gyro parameters into ArConfig
  ArRobotConfig robotConfig(&robot);
  robotConfig.addAnalogGyro(&gyro);
//*****************************
  
  
  robot.enableMotors();
  robot.clearDirectMotion();

  // if we are connected to a simulator, reset it to its start position
  robot.comInt(ArCommands::RESETSIMTOORIGIN, 1);
  robot.moveTo(ArPose(0,0,0));


  // Set up laser using connector (command line arguments, etc.)
   simpleConnector.setupLaser(&sick);

  // Start the robot thread.
  robot.runAsync(true);
  
  // Start the laser thread.
  sick.runAsync();

  // Try to connect the laser
  if (!sick.blockingConnect())
    ArLog::log(ArLog::Normal, "Warning: Couldn't connect to SICK laser, it won't be used");
  else
    ArLog::log(ArLog::Normal, "Connected to laser.");

//***************************************
  // Add additional range devices to the robot and path planning task.
  // IRs if the robot has them.
  robot.lock();
//   ArIRs irs;
//   robot.addRangeDevice(&irs);
//   pathTask.addRangeDevice(&irs, ArPathPlanningTask::CURRENT);
//******************************************


  // Forbidden regions from the map
  ArForbiddenRangeDevice forbidden(&arMap);
  robot.addRangeDevice(&forbidden);

  // This is the place to add a range device which will hold sensor data
  // and delete it appropriately to replan around blocked paths.
  ArGlobalReplanningRangeDevice replanDev(&pathTask);

  // Create objects that add network services:
  
  // Drawing in the map display:
  ArServerInfoDrawings drawings(&server);
  drawings.addRobotsRangeDevices(&robot);
  drawings.addRangeDevice(&replanDev);

  /* If you want to draw the destination put this code back in:
  ArServerDrawingDestination destination(
	  &drawings, &pathTask, "destination",
	  500, 500,
	  new ArDrawingData("polyDots",
			    ArColor(0xff, 0xff, 0x0),
			    800, // size
			    49), // just below the robot
  */ 

  /* If you want to see the local path planning area use this 
    (You can enable this particular drawing from custom commands 
    which is set up down below in ArServerInfoPath) 
  ArDrawingData drawingDataP("polyLine", ArColor(200,200,200), 1, 75);
  ArFunctor2C<ArPathPlanningTask, ArServerClient *, ArNetPacket *> 
  drawingFunctorP(pathTask, &ArPathPlanningTask::drawSearchRectangle);
  drawings.addDrawing(&drawingDataP, "Local Plan Area", &drawingFunctorP); 
  */

  /* If you want to see the points making up the local path in addition to the
   * main path use this. 
  ArDrawingData drawingDataP2("polyDots", ArColor(0,128,0), 100, 70);
  ArFunctor2C<ArPathPlanningTask, ArServerClient *, ArNetPacket *> 
  drawingFunctorP2(pathTask, &ArPathPlanningTask::drawPathPoints);
  drawings.addDrawing(&drawingDataP2, "Path Points", &drawingFunctorP2);
  */

  // Misc. simple commands:
  ArServerHandlerCommands commands(&server);
	

  // These provide various kinds of information to the client:
  ArServerInfoRobot serverInfoRobot(&server, &robot);
  ArServerInfoSensor serverInfoSensor(&server, &robot);
  ArServerInfoPath serverInfoPath(&server, &robot, &pathTask);
  serverInfoPath.addSearchRectangleDrawing(&drawings);
  serverInfoPath.addControlCommands(&commands);
//-------------------------receive commands or events from client---------------------
  
//   ArServerHandlerCommands commands(&server);
	server.addData("RobotVideo"			,"", new ArGlobalFunctor2<ArServerClient *, ArNetPacket*>(&VideoServerBase::RobotVideoCB) ,"","");
	server.addData("turn"						,"", new ArGlobalFunctor2<ArServerClient *, ArNetPacket*>(&turn_func) ,"","");
	server.addData("RobotMotion"	  ,"", new ArGlobalFunctor2<ArServerClient *, ArNetPacket*>(&S_RobotMotion) ,"","");
	server.addData("CameraMotion"   ,"", new ArGlobalFunctor2<ArServerClient *, ArNetPacket*>(&S_CameraMotion) ,"","");
	server.addData("RobotTurnLeft"  ,"", new ArGlobalFunctor2<ArServerClient *, ArNetPacket*>(&S_RobotTurnLeft) ,"","");
	server.addData("RobotTurnRight" ,"", new ArGlobalFunctor2<ArServerClient *, ArNetPacket*>(&S_RobotTurnRight) ,"","");
	server.addData("TargetApproach" ,"", new ArGlobalFunctor2<ArServerClient *, ArNetPacket*>(&S_TargetApproach) ,"","");
	server.addData("TargetApproachObstacles" ,"", new ArGlobalFunctor2<ArServerClient *, ArNetPacket*>(&S_TargetApproach_Obstacles) ,"","");
	server.addData("GlassesCancel" ,"", new ArGlobalFunctor2<ArServerClient *, ArNetPacket*>(&S_GlassesCancel) ,"","");
	server.addData("Calibration" ,"", new ArGlobalFunctor2<ArServerClient *, ArNetPacket*>(&S_Calibration) ,"","");

	server.addClientRemovedCallback(new ArGlobalFunctor1< ArServerClient * >(&clientCloseCallback));
//-------------------------receive commands or events from client---------------------


  
//***********************  
  ArServerInfoLocalization serverInfoLocalization (&server, &robot, &locManager);
  ArServerHandlerLocalization serverLocHandler (&server, &robot, &locManager);


  // Provide the map to the client (and related controls):
  // This uses both lines and points now, since everything except
  // sonar localization uses both (path planning with sonar still uses both)
  ArServerHandlerMap serverMap(&server, &arMap);

  

  // Add some simple (custom) commands for testing and debugging:
  ArServerSimpleComUC uCCommands(&commands, &robot);                   // Send any command to the microcontroller
  ArServerSimpleComMovementLogging loggingCommands(&commands, &robot); // configure logging
  ArServerSimpleComGyro gyroCommands(&commands, &robot, &gyro);        // monitor the gyro
  ArServerSimpleComLogRobotConfig configCommands(&commands, &robot);   // trigger logging of the robot config parameters
  ArServerSimpleServerCommands serverCommands(&commands, &server);     // monitor networking behavior (track packets sent etc.)


  /* Set up the possible modes for remote control from a client such as
   * MobileEyes:
   */

  // Mode To go to a goal or other specific point:
  ArServerModeGoto modeGoto(&server, &robot, &pathTask, &arMap, ArPose(0,0,0));

  // Add a simple (custom) command that allows you to give a list of 
  // goals to tour, instead of all. Useful for testing and debugging.
  modeGoto.addTourGoalsInListSimpleCommand(&commands);

  // Mode To stop and remain stopped:
  ArServerModeStop modeStop(&server, &robot);

  // cause the sonar to turn off automatically
  // when the robot is stopped, and turn it back on when commands to move
  // are sent. (Note, this should not be done if you need the sonar
  // data to localize, or for other purposes while stopped)
  ArSonarAutoDisabler sonarAutoDisabler(&robot);

  // Teleoperation modes To drive by keyboard, joystick, etc:
  ArServerModeRatioDrive modeRatioDrive(&server, &robot);  // New, improved mode
  ArServerModeDrive modeDrive(&server, &robot);            // Older mode for compatability

  // Drive mode's configuration and custom (simple) commands:
  modeRatioDrive.addToConfig(Aria::getConfig(), "Teleop settings");
  modeDrive.addControlCommands(&commands);
  modeRatioDrive.addControlCommands(&commands);

  // Wander mode 
//   ArServerModeWander modeWander(&server, &robot);
//*********************************
  // Prevent driving if localization is lost:
  ArActionLost actionLostRatioDrive (&locManager, NULL, &modeRatioDrive);
  modeRatioDrive.getActionGroup ()->addAction (&actionLostRatioDrive, 110);

  // Prevent wandering if lost:
//   ArActionLost
//   actionLostWander (&locManager, NULL, &modeWander);
//   modeWander.getActionGroup ()->addAction (&actionLostWander, 110);

  // This provides a small table of interesting information for the client
  // to display to the operator:
  ArServerInfoStrings stringInfo(&server);
  Aria::getInfoGroup()->addAddStringCallback(stringInfo.getAddStringFunctor());
  
  Aria::getInfoGroup()->addStringInt(
	  "Motor Packet Count", 10, 
	  new ArConstRetFunctorC<int, ArRobot>(&robot, 
					       &ArRobot::getMotorPacCount));


  // Make Stop mode the default (If current mode deactivates without entering
  // a new mode, then Stop Mode will be selected)
  modeStop.addAsDefaultMode();






  // Create the service that allows the client to monitor the communication 
  // between the robot and the client.
  //
  ArServerHandlerCommMonitor handlerCommMonitor(&server);

  // Create service that allows client to change configuration parameters in ArConfig 
  ArServerHandlerConfig handlerConfig(&server, Aria::getConfig(),
				      Arnl::getTypicalDefaultParamFileName(),
				      Aria::getDirectory());



  // Read in parameter files.  read the paras from input
  Aria::getConfig()->useArgumentParser(&parser);
  if (!Aria::getConfig()->parseFile(Arnl::getTypicalParamFileName()))
  {
    ArLog::log(ArLog::Normal, "Trouble loading configuration file, exiting");
    Aria::exit(5);
  }

  // Warn about unknown params.
  if (!simpleOpener.checkAndLog() || !parser.checkHelpAndWarnUnparsed())
  {
    ArLog::log(ArLog::Normal, "\nUsage: %s -map mapfilename\n", argv[0]);
    simpleConnector.logOptions();
    simpleOpener.logOptions();
    Aria::exit(6);
  }

  // Warn if there is no map
  if (arMap.getFileName() == NULL || strlen(arMap.getFileName()) <= 0)
  {
    ArLog::log(ArLog::Normal, "");
    ArLog::log(ArLog::Normal, "### Warning, No map file is set up, you can make a map with sickLogger or arnlServer, and Mapper3; More info in docs/Mapping.txt and README.txt. Set the map with the -map command line option, or by changing the config with MobileEyes or by editing the config file.");
    ArLog::log(ArLog::Normal, "");    
  }

  // find out where we'll want to put files
  ArLog::log(ArLog::Normal, "");
  ArLog::log(ArLog::Normal, 
	     "Directory for maps and file serving: %s", fileDir);
  
  ArLog::log(ArLog::Normal, "See the ARNL README.txt for more information");
  ArLog::log(ArLog::Normal, "");

  // If you want MobileSim to try and load up the same map as you are
  // using in guiServer then uncomment out the next line and this object
  // will send a command to MobileSim to do so, but make sure you start 
  // MobileSim from the Arnl/examples directory or use the --cwd option, 
  // so that the map names used by MobileSim match  the map names used 
  // by guiServer
  //ArSimMapSwitcher mapSwitcher(&robot, &arMap);

/******************************************************
 * ****************************************************
 * 			Camera 
 * ****************************************************
 * *****************************************************/
	
  
  robot.unlock();
    // Localize robot at home.
  locTask.localizeRobotAtHomeBlocking();

  locTask.forceUpdatePose(ArPose(0,0,0));

  resetMotion();
  

	

// robot.enableMotors();
// robot.runAsync(true);
  //locTask.localizeRobotAtHomeBlocking();
  server.runAsync();

  VideoServerBase videoserver;
  videoserver.runAsync();

	robot.comInt(ArCommands::SOUNDTOG, 0);
	
	//G_PathPlanning->pathPlanToPose(ArPose(1500, -1500 , -32),true,true);
	//while(G_PathPlanning->getState() != ArPathPlanningTask::REACHED_GOAL );

	//S_TargetApproach_Obstacles1();

	//ArUtil::sleep(5000);


 //   G_PTZHandler->panRel(60);
	//	ArUtil::sleep(3000);
	//while(1)
	//S_TargetApproach1();
	cout << "done!!!!!!!---------------------" <<endl;
  robot.waitForRunExit();
  Aria::exit(0);
}

