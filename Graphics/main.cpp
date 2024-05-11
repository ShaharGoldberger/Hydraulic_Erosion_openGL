#define _CRT_SECURE_NO_WARNINGS
#include <stdlib.h>
#include <stdio.h>
#include <vector>
#include <stack>
#include <time.h>
#include <math.h>
#include "glut.h"
#include <iostream>
#include <cmath>

const double PI = 3.14;
const int GSZ = 100; // Grid size for the terrain
const int GRID_HALF_SIZE = 50; // Half of the grid size GSZ is 100
const int H = 600;
const int W = 600;
const int TH = 512;
const int TW = 512;

unsigned char tx0[TH][TW][3];

double angle = 0;

typedef struct {
	double x, y, z;
} POINT3;

struct Point {
	int x, z;
};

POINT3 eye = { 2,14,10 };

double sight_angle = PI;
POINT3 sight_dir = { sin(sight_angle),0,cos(sight_angle) }; // in plane X-Z
double speed = 0;
double angular_speed = 0;

// aircraft defs
double air_sight_angle = PI;
POINT3 air_sight_dir = { sin(air_sight_angle),0,cos(air_sight_angle) }; // in plane X-Z
double air_speed = 0;
double air_angular_speed = 0;
POINT3 aircraft = { 0,15,0 };
double pitch = 0;

double ground[GSZ][GSZ] = { 0 };
double startingGround[GSZ][GSZ] = { 0 };
double waterLevel[GSZ][GSZ] = { 0 };
double tempTerrainHeight[GSZ][GSZ];
int terrainCheckRadius = 1;
bool isSelectionActive = false;
const double erosionDepth = 0.4;
bool runErosion = true;
int maxBuildingHeight = 4;
int minBuildingHeight = 2;
int maxDistanceFromCenter = 8;
int minRiverDistance = 1;
int maxRiverDistance = 3;

std::vector<std::vector<bool>> visitedCells(GSZ, std::vector<bool>(GSZ, false));
std::vector<std::vector<int>>gridBuildingHeights(100, std::vector<int>(GSZ, 0));
std::vector<std::vector<bool>> buildableAreas(GSZ, std::vector<bool>(GSZ, false));


// lighting
float lt1amb[4] = { 0.3 ,0.3,0.3, 0 };
float lt1diff[4] = { 0.6 ,0.6,0.6, 0 };
float lt1spec[4] = { 0.8 ,0.8,0.8, 0 };
float lt1pos[4] = { 1 ,1,0.2, 0 };


float lt2amb[4] = { 0.3 ,0.3,0.3, 0 };
float lt2diff[4] = { 0.6 ,0.1,0.6, 0 };
float lt2spec[4] = { 0.8 ,0.8,0.8, 0 };
float lt2pos[4] = { -1 ,-0.3,0.2, 0 };

// materials
// silver
float mt1amb[4] = { 0.5 ,0.5,0.5, 0 };
float mt1diff[4] = { 0.8 ,0.8,0.8, 0 };
float mt1spec[4] = { 1 ,1,1, 0 };

// gold
float mt2amb[4] = { 0.2 ,0.2,0.0, 0 };
float mt2diff[4] = { 0.7 ,0.6,0.2, 0 };
float mt2spec[4] = { 1 ,1,1, 0 };

// glass
float mt3amb[4] = { 0.0 ,0.0,0.0, 0.5 };
float mt3diff[4] = { 0.2 ,0.4,0.5, 0.5 };
float mt3spec[4] = { 1 ,1,1, 0.5 };

void UpdateGround2();
void UpdateGround3();
void Smooth();

unsigned int letters;


void menu(int choice);
void keyboard(unsigned char key, int x, int y);
void mouseMotion(int x, int y);
void mouse(int button, int state, int x, int y);
void SpecialKeys(int key, int x, int y);
void idle();
void displayCity();
void displayCombined();
void displayCockpit();
void displayTop();
void display();
void DrawColorCube();
void displayMap();
bool iterativeFloodFillSearchAlgorithm(int centerX, int centerZ);
bool isAreaSuitableForBuilding(int centerX, int centerZ);
bool isUnsuitableForBuilding(int x, int z);
bool isWithinBounds(int x, int z);
bool isSuitableForConstruction(int targetX, int targetZ);
bool isTerrainUnsuitableForConstruction(int posX, int posZ);
void DrawFloor();
void SetColor(double h);
void Smooth();
void UpdateGround1();
void UpdateGround2();
void UpdateGround3();
void init();
void SetTexture(int tnum);
void ShowText(char* text);
void initFont();
void drawBuilding(int x, int z);
void drawFacade(int baseX, int baseZ, int limitX, int limitZ, double facadeHeight, int totalFloors, double windowShift);
void setWallColor();
void setWindowColor();
void setRoofColor();
void SimulateHydraulicErosion(int iterations, int raindrops_per_iteration, double erosion_factor, double min_water_volume);
void drawWallForBuilding(int initialX, int initialZ, int finalX, int finalZ, double maxHeight);
void drawWindow(double startX, double startZ, double endX, double endZ, double baseHeight, double topHeight);
void drawWindowsForBuildings(int baseX, int baseZ, int limitX, int limitZ, double peakHeight, int numFloors, double offset);
void drwaRoofForBuilding(int posX, int posZ, double baseHeight, double peakIncrement);


void main(int argc, char* argv[])
{
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH);
	glutInitWindowSize(W, H);
	glutInitWindowPosition(400, 100);
	glutCreateWindow("Hydraulic Errosion");

	glutDisplayFunc(display);
	glutIdleFunc(idle);
	glutSpecialFunc(SpecialKeys);
	glutMouseFunc(mouse);
	glutKeyboardFunc(keyboard);
	glutMotionFunc(mouseMotion);

	glutCreateMenu(menu);
	glutAddMenuEntry("Regular view", 1);
	glutAddMenuEntry("Top view", 2);
	glutAddMenuEntry("Cockpit view", 3);
	glutAddMenuEntry("Combined view", 4);
	glutAddMenuEntry("Build city", 5);

	glutAttachMenu(GLUT_RIGHT_BUTTON);

	init();

	glutMainLoop();
}


void menu(int choice)
{
	switch (choice)
	{
	case 1: // regular view
		glutDisplayFunc(display);
		runErosion = true;  // Continue erosion in regular view
		break;
	case 2: // top view
		glutDisplayFunc(displayTop);
		runErosion = true;  // Continue erosion in regular view
		break;
	case 3: // cockpit view
		glutDisplayFunc(displayCockpit);
		runErosion = true;  // Continue erosion in regular view
		break;
	case 4: // combined view
		glutDisplayFunc(displayCombined);
		runErosion = true;  // Continue erosion in regular view
		break;
	case 5: // build city
		glutDisplayFunc(displayCity);  // You will need to define this function
		runErosion = false;

		break;
	}
}

void keyboard(unsigned char key, int x, int y)
{
	switch (key)
	{
	case 'w':
		air_speed += 0.01;
		break;
	case 's':
		air_speed -= 0.01;
		break;
	case 'a':
		air_angular_speed += 0.001; // yaw
		break;
	case 'd':
		air_angular_speed -= 0.001; // yaw
		break;
	}
}

void mouseMotion(int x, int y)
{
	// updates pitch
	if (isSelectionActive)
	{
		if ((H - y) > 5 && (H - y) < 95)
			pitch = (2 * (H - y) / 100.0) - 1;
	}
}

void mouse(int button, int state, int x, int y)
{
	if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN)
	{
		double h = 100 * (1 + pitch) / 2; // middle of the slider button

		if (x > 42 && x<58 && (H - y)>h - 8 && (H - y) < h + 8)
			isSelectionActive = true;
	}
	else if (button == GLUT_LEFT_BUTTON && state == GLUT_UP)
	{
		isSelectionActive = false; // release mouse
	}
}

void SpecialKeys(int key, int x, int y)
{
	switch (key)
	{
	case GLUT_KEY_LEFT: // turns the user to the left
		angular_speed += 0.0004;
		break;
	case GLUT_KEY_RIGHT:
		angular_speed -= 0.0004;
		break;
	case GLUT_KEY_UP: // increases the speed
		speed += 0.005;
		break;
	case GLUT_KEY_DOWN:
		speed -= 0.005;
		break;
	case GLUT_KEY_PAGE_UP:
		eye.y += 0.1;
		break;
	case GLUT_KEY_PAGE_DOWN:
		eye.y -= 0.1;
		break;

	}
}

void idle()
{
	int i, j;
	double dist;
	angle += 0.1;

	// aircraft motion
	air_sight_angle += air_angular_speed;

	air_sight_dir.x = sin(air_sight_angle);
	air_sight_dir.y = sin(-pitch);
	air_sight_dir.z = cos(air_sight_angle);

	aircraft.x += air_speed * air_sight_dir.x;
	aircraft.y += air_speed * air_sight_dir.y;
	aircraft.z += air_speed * air_sight_dir.z;

	// ego-motion  or locomotion
	sight_angle += angular_speed;
	// the direction of our sight (forward)
	sight_dir.x = sin(sight_angle);

	sight_dir.z = cos(sight_angle);
	// motion
	eye.x += speed * sight_dir.x;
	eye.y += speed * sight_dir.y;
	eye.z += speed * sight_dir.z;

	if (runErosion) {
		// Only run the simulation if runErosion is true
		SimulateHydraulicErosion(1, 100, 0.001, 0.01);
	}

	glutPostRedisplay();
}

void displayCity()
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glViewport(0, 0, W, H);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glFrustum(-1, 1, -1, 1, 0.75, 300);
	gluLookAt(eye.x, eye.y, eye.z, eye.x + sight_dir.x, eye.y - 0.3, eye.z + sight_dir.z, 0, 1, 0);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	DrawFloor();

	displayMap();

	glutSwapBuffers();
}

void displayCombined()
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // clean frame buffer and Z-buffer

	glViewport(0, H / 2, W / 2, H / 2);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity(); // unity matrix of projection

	glFrustum(-1, 1, -1, 1, 0.75, 300);
	gluLookAt(eye.x, eye.y, eye.z,  // eye position
		eye.x + sight_dir.x, eye.y - 0.3, eye.z + sight_dir.z,  // sight dir
		0, 1, 0);


	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity(); // unity matrix of model

	DrawFloor();

	glPushMatrix();
	glTranslated(aircraft.x, aircraft.y, aircraft.z);
	glRotated(90 + air_sight_angle * 180 / PI, 0, 1, 0);
	glRotated((air_angular_speed) * 5000, 1, 0, 0);
	glPopMatrix();


	// top view
	glViewport(W / 2, H / 2, W / 2, H / 2);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity(); // unity matrix of projection

	glFrustum(-1, 1, -1, 1, 0.75, 300);
	gluLookAt(eye.x, 70, eye.z,  // eye position
		eye.x + sight_dir.x, 60, eye.z + sight_dir.z,  // sight dir
		0, 1, 0);


	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity(); // unity matrix of model

	DrawFloor();

	glPushMatrix();
	glTranslated(aircraft.x, aircraft.y, aircraft.z);
	glRotated(90 + air_sight_angle * 180 / PI, 0, 1, 0);
	glRotated((air_angular_speed) * 5000, 1, 0, 0);
	glScaled(0.4, 0.4, 0.4);

	glPopMatrix();

	// cocpit view
	glViewport(0, 0, W, H / 2);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity(); // unity matrix of projection

	glFrustum(-1, 1, -1, 1, 0.75, 300);
	gluLookAt(aircraft.x, aircraft.y + 4, aircraft.z,  // eye position
		aircraft.x + air_sight_dir.x, aircraft.y + 3.8, aircraft.z + air_sight_dir.z,  // sight dir
		0, 1, 0);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity(); // unity matrix of model

	DrawFloor();

	glPushMatrix();
	glTranslated(aircraft.x, aircraft.y, aircraft.z);

	glRotated(90 + air_sight_angle * 180 / PI, 0, 1, 0);
	glRotated((air_angular_speed) * 5000, 1, 0, 0);
	glPopMatrix();


	glutSwapBuffers(); // show all
}

void displayCockpit()
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // clean frame buffer and Z-buffer

	glViewport(0, 0, W, H);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity(); // unity matrix of projection

	glFrustum(-1, 1, -1, 1, 0.75, 300);
	gluLookAt(aircraft.x, aircraft.y + 4, aircraft.z,  // eye position
		aircraft.x + air_sight_dir.x, aircraft.y + 3.8, aircraft.z + air_sight_dir.z,  // sight dir
		0, 1, 0);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity(); // unity matrix of model

	DrawFloor();

	glPushMatrix();
	glTranslated(aircraft.x, aircraft.y, aircraft.z);

	glRotated(90 + air_sight_angle * 180 / PI, 0, 1, 0);
	glRotated(pitch * 180 / PI, 0, 0, 1); // pitch
	glRotated((air_angular_speed) * 5000, 1, 0, 0);
	glPopMatrix();

	glutSwapBuffers(); // show all
}

void displayTop()
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // clean frame buffer and Z-buffer
	glViewport(0, 0, W, H);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity(); // unity matrix of projection

	glFrustum(-1, 1, -1, 1, 0.75, 300);
	gluLookAt(eye.x, 70, eye.z,  // eye position
		eye.x + sight_dir.x, 60, eye.z + sight_dir.z,  // sight dir
		0, 1, 0);


	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity(); // unity matrix of model

	DrawFloor();

	glPushMatrix();
	glTranslated(aircraft.x, aircraft.y, aircraft.z);
	glRotated(90 + air_sight_angle * 180 / PI, 0, 1, 0);
	glRotated((air_angular_speed) * 5000, 1, 0, 0);
	glScaled(0.4, 0.4, 0.4);

	glPopMatrix();

	glutSwapBuffers(); // show all
}

void display()
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // clean frame buffer and Z-buffer
	glViewport(0, 0, W, H);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity(); // unity matrix of projection

	glFrustum(-1, 1, -1, 1, 0.75, 300);
	gluLookAt(eye.x, eye.y, eye.z,  // eye position
		eye.x + sight_dir.x, eye.y - 0.3, eye.z + sight_dir.z,  // sight dir
		0, 1, 0);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity(); // unity matrix of model

	DrawFloor();

	glutSwapBuffers(); // show all
}

void DrawColorCube()
{
	// bottom plane
	glBegin(GL_POLYGON);
	glColor3d(0, 0, 1); // blue
	glVertex3d(1, -1, 1);
	glColor3d(1, 0, 1); // magenta
	glVertex3d(1, -1, -1);
	glColor3d(0, 0, 0); // black
	glVertex3d(-1, -1, -1);
	glColor3d(0, 1, 1); // Cyan
	glVertex3d(-1, -1, 1);
	glEnd();

	// top plane
	glBegin(GL_POLYGON);
	glColor3d(1, 1, 1); // white
	glVertex3d(1, 1, 1);
	glColor3d(1, 0, 0); // red
	glVertex3d(1, 1, -1);
	glColor3d(1, 1, 0); // yellow
	glVertex3d(-1, 1, -1);
	glColor3d(0, 1, 0); // green
	glVertex3d(-1, 1, 1);
	glEnd();

	//rear plane
	glBegin(GL_LINE_LOOP);//GL_POLYGON);
	glColor3d(1, 0, 0); // red
	glVertex3d(1, 1, -1);
	glColor3d(1, 1, 0); // yellow
	glVertex3d(-1, 1, -1);
	glColor3d(0, 0, 0); // black
	glVertex3d(-1, -1, -1);
	glColor3d(1, 0, 1); // magenta
	glVertex3d(1, -1, -1);

	glEnd();

	//left
	glBegin(GL_LINE_LOOP);//GL_POLYGON);

	glColor3d(1, 1, 0); // yellow
	glVertex3d(-1, 1, -1);
	glColor3d(0, 1, 0); // green
	glVertex3d(-1, 1, 1);
	glColor3d(0, 1, 1); // Cyan
	glVertex3d(-1, -1, 1);
	glColor3d(0, 0, 0); // black
	glVertex3d(-1, -1, -1);
	glEnd();

	// right
	glBegin(GL_LINE_LOOP);//GL_POLYGON);
	glColor3d(1, 1, 1); // white
	glVertex3d(1, 1, 1);
	glColor3d(1, 0, 0); // red
	glVertex3d(1, 1, -1);
	glColor3d(1, 0, 1); // magenta
	glVertex3d(1, -1, -1);
	glColor3d(0, 0, 1); // blue
	glVertex3d(1, -1, 1);

	glEnd();

	// front
	glBegin(GL_LINE_LOOP);//GL_POLYGON);
	glColor3d(1, 1, 1); // white
	glVertex3d(1, 1, 1);
	glColor3d(0, 1, 0); // green
	glVertex3d(-1, 1, 1);
	glColor3d(0, 1, 1); // Cyan
	glVertex3d(-1, -1, 1);
	glColor3d(0, 0, 1); // blue
	glVertex3d(1, -1, 1);
	glEnd();
}

void displayMap()
{
	int buildingsInitiated = 0;
	DrawFloor();

	for (int attempt = 0; attempt < 10; attempt++)
	{
		int attemptsLimit = 755, attemptsCount = 0;
		bool suitableLocationFound = false;
		int posZ;
		int posX;

		while (attemptsCount < attemptsLimit && !suitableLocationFound)
		{
			while (true)
			{
				posX = std::rand() % GSZ;
				posZ = std::rand() % GSZ;
				if (ground[posX][posZ] >= 0.2 && ground[posX][posZ] >= startingGround[posX][posZ] - erosionDepth * startingGround[posX][posZ])
				{
					break;
				}
			}

			attemptsCount++;
			suitableLocationFound = iterativeFloodFillSearchAlgorithm(posX, posZ);
		}
		if (suitableLocationFound)
		{
			buildingsInitiated++;
		}
	}

	for (int i = 0; i < GSZ; i++)
	{
		for (int j = 0; j < GSZ; j++)
		{
			if (buildableAreas[i][j] == true && gridBuildingHeights[i][j] > 0)
			{
				drawBuilding(i, j);
				glPushMatrix();
				glPopMatrix();
			}
		}
	}
}

bool iterativeFloodFillSearchAlgorithm(int centerX, int centerZ)
{
	bool foundValidLocation = false;
	std::vector<Point> processingStack;
	Point current = { centerX, centerZ };
	processingStack.push_back(current);

	while (!processingStack.empty())
	{
		current = processingStack.back();
		processingStack.pop_back();

		if (isSuitableForConstruction(current.x, current.z) && isAreaSuitableForBuilding(current.x, current.z))
		{
			float red = static_cast<float>(rand() % 256) / 255.0f;
			float green = static_cast<float>(rand() % 256) / 255.0f;
			float blue = static_cast<float>(rand() % 256) / 255.0f;
			glColor3f(red, green, blue);

			int extendedRadius = terrainCheckRadius + 1;
			int startX = std::max(0, current.x - extendedRadius);
			int endX = std::min(GSZ - 1, current.x + extendedRadius);
			int startZ = std::max(0, current.z - extendedRadius);
			int endZ = std::min(GSZ - 1, current.z + extendedRadius);

			for (int i = startX; i <= endX; i++)
			{
				for (int j = startZ; j <= endZ; j++)
				{
					visitedCells[i][j] = true;
					buildableAreas[i][j] = true;
				}
			}
			int heightRange = maxBuildingHeight - minBuildingHeight + 1;
			gridBuildingHeights[current.x][current.z] = minBuildingHeight + rand() % heightRange;

			foundValidLocation = true;
		}
	}

	return foundValidLocation;
}

bool isAreaSuitableForBuilding(int centerX, int centerZ)
{
	int radius = terrainCheckRadius;
	bool isAreaSuitable = true;

	for (int dx = -radius; dx <= radius; dx++)
	{
		for (int dz = -radius; dz <= radius; dz++)
		{
			int currentX = centerX + dx;
			int currentZ = centerZ + dz;

			if (!isWithinBounds(currentX, currentZ) || isUnsuitableForBuilding(currentX, currentZ))
			{
				isAreaSuitable = false;
				break;
			}
		}
		if (!isAreaSuitable)
		{
			break;
		}
	}

	return isAreaSuitable;
}

bool isUnsuitableForBuilding(int x, int z)
{
	return (ground[x][z] <= 0.2 || ground[x][z] <= startingGround[x][z] - erosionDepth * startingGround[x][z] || buildableAreas[x][z]);
}

bool isWithinBounds(int x, int z)
{
	return (x >= 0 && x < GSZ && z >= 0 && z < GSZ);
}

bool isSuitableForConstruction(int targetX, int targetZ)
{
	int minDistSq = minRiverDistance * minRiverDistance;
	int maxDistSq = maxRiverDistance * maxRiverDistance;
	bool isSuitable = false;

	for (int offsetX = -maxRiverDistance; offsetX <= maxRiverDistance; offsetX++)
	{
		for (int offsetZ = -maxRiverDistance; offsetZ <= maxRiverDistance; offsetZ++)
		{
			int distanceSquared = offsetX * offsetX + offsetZ * offsetZ;
			if (distanceSquared >= minDistSq && distanceSquared <= maxDistSq)
			{
				int posX = targetX + offsetX;
				int posZ = targetZ + offsetZ;

				if (isTerrainUnsuitableForConstruction(posX, posZ))
				{
					isSuitable = true;
					break;
				}
			}
		}
		if (isSuitable)
		{
			break;
		}
	}

	return isSuitable;
}

bool isTerrainUnsuitableForConstruction(int posX, int posZ)
{
	double newModifiedGroundLevel = startingGround[posX][posZ] - erosionDepth * startingGround[posX][posZ];

	return ground[posX][posZ] < newModifiedGroundLevel || ground[posX][posZ] < 0;
}

void DrawFloor()
{
	int i, j;

	glColor3d(0, 0, 0.3);

	for (i = 1; i < GSZ - 1; i++)
	{
		for (j = 1; j < GSZ - 1; j++)
		{
			glBegin(GL_POLYGON);
			SetColor(ground[i][j]);
			glVertex3d(j - GSZ / 2, ground[i][j], i - GSZ / 2);
			SetColor(ground[i - 1][j]);
			glVertex3d(j - GSZ / 2, ground[i - 1][j], i - 1 - GSZ / 2);
			SetColor(ground[i - 1][j - 1]);
			glVertex3d(j - 1 - GSZ / 2, ground[i - 1][j - 1], i - 1 - GSZ / 2);
			SetColor(ground[i][j - 1]);
			glVertex3d(j - 1 - GSZ / 2, ground[i][j - 1], i - GSZ / 2);
			glEnd();
		}
	}

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glColor4d(0, 0.4, 0.7, 0.7);
	glBegin(GL_POLYGON);
	glVertex3d(-GSZ / 2, 0, -GSZ / 2);
	glVertex3d(-GSZ / 2, 0, GSZ / 2);
	glVertex3d(GSZ / 2, 0, GSZ / 2);
	glVertex3d(GSZ / 2, 0, -GSZ / 2);
	glEnd();

	glDisable(GL_BLEND);
}

void SetColor(double h)
{
	h = fabs(h) / 6;
	// sand
	if (h < 0.02)
		glColor3d(0.8, 0.7, 0.5);
	else	if (h < 0.3)// grass
		glColor3d(0.4 + 0.7 * h, 0.6 - 0.6 * h, 0.2 + 0.2 * h);
	else if (h < 0.6) // stones
		glColor3d(0.4 + 0.25 * h, 0.4 + 0.18 * h, 0.3 + 0.13 * h);
	else // snow
		glColor3d(h, h, 1.1 * h);

}

void Smooth()
{
	for (int i = 1; i < GSZ - 1; i++)
		for (int j = 1; j < GSZ - 1; j++)
		{
			tempTerrainHeight[i][j] = (ground[i - 1][j - 1] + ground[i - 1][j] + ground[i - 1][j + 1] +
				ground[i][j - 1] + ground[i][j] + ground[i][j + 1] +
				ground[i + 1][j - 1] + ground[i + 1][j] + ground[i + 1][j + 1]) / 9.0;
		}

	for (int i = 1; i < GSZ - 1; i++)
		for (int j = 1; j < GSZ - 1; j++)
			ground[i][j] = tempTerrainHeight[i][j];

}

void UpdateGround1()
{
	double delta = 0.2;
	if (rand() % 2 == 0)
		delta = -delta;
	int x, z;

	for (int i = 0; i < 100; i++) {
		x = rand() % GSZ;
		z = rand() % GSZ;

		ground[z][x] += delta;
	}
}

void UpdateGround2()
{
	double delta = 0.04;
	if (rand() % 2 == 0)
		delta = -delta;
	int x, z, num_pts = 5000;
	x = rand() % GSZ;
	z = rand() % GSZ;

	for (int i = 0; i < num_pts; i++) {
		ground[z][x] += delta;
		switch (rand() % 4)
		{
		case 0: // right
			x++;
			break;
		case 1: // left
			x--;
			break;
		case 2: // down
			z++;
			break;
		case 3: // up
			z--;
			break;
		}
		if (x > GSZ) x -= GSZ;
		if (x < 0) x += GSZ;
		if (z > GSZ) z -= GSZ;
		if (z < 0) z += GSZ;
	}
}

void UpdateGround3()
{
	double delta = 0.04;
	if (rand() % 2 == 0)
		delta = -delta;
	int x1, y1, x2, y2;
	x1 = rand() % GSZ;
	y1 = rand() % GSZ;
	x2 = rand() % GSZ;
	y2 = rand() % GSZ;
	double a, b;
	if (x1 != x2)
	{
		a = (y2 - y1) / ((double)x2 - x1);
		b = y1 - a * x1;
		for (int i = 0; i < GSZ; i++)
			for (int j = 0; j < GSZ; j++)
			{
				if (i < a * j + b) ground[i][j] += delta;
				else ground[i][j] -= delta;
			}
	}
}

void init()
{
	//                 R     G    B
	glClearColor(0.5, 0.7, 1, 0);// color of window background

	glEnable(GL_DEPTH_TEST);

	int i, j;
	double dist;

	srand(time(0));

	for (i = 0; i < 3000; i++)
		UpdateGround3();

	Smooth();
	for (i = 0; i < 1000; i++)
		UpdateGround3();

	// flatten terrain below road
	for (j = 0; j < GSZ; j++)
	{
		ground[GSZ / 2 - 1][j] = ground[GSZ / 2 + 1][j] = ground[GSZ / 2][j];
	}

	glEnable(GL_NORMALIZE);

	SetTexture(1); // Road
	glBindTexture(GL_TEXTURE_2D, 1);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, TW, TH, 0, GL_RGB, GL_UNSIGNED_BYTE, tx0);

	SetTexture(2); // cross walk
	glBindTexture(GL_TEXTURE_2D, 2);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, TW, TH, 0, GL_RGB, GL_UNSIGNED_BYTE, tx0);

	// Simulate hydraulic erosion
	SimulateHydraulicErosion(100, 1000, 0.001, 0.01);
}

void SetTexture(int tnum)
{
	int i, j;
	int tmp;
	if (tnum == 1) // road texture
	{
		for (i = 0; i < TH; i++)
			for (j = 0; j < TW; j++)
			{
				tmp = rand() % 20;
				if (i<20 || i>TH - 20 || fabs(TH / 2 - i) < 10 && j < TW / 2) // yellow
				{
					tx0[i][j][0] = 220 + tmp;
					tx0[i][j][1] = 220 + tmp;
					tx0[i][j][2] = 0;
				}
				else // gray
				{
					tx0[i][j][0] = 150 + tmp;
					tx0[i][j][1] = 150 + tmp;
					tx0[i][j][2] = 150 + tmp;
				}
			}
	}
	if (tnum == 2) // cross walk texture
	{
		for (i = 0; i < TH; i++)
			for (j = 0; j < TW; j++)
			{
				tmp = rand() % 20;
				if (i < TH / 2) // yellow
				{
					tx0[i][j][0] = 220 + tmp;
					tx0[i][j][1] = 220 + tmp;
					tx0[i][j][2] = 0;
				}
				else // gray
				{
					tx0[i][j][0] = 150 + tmp;
					tx0[i][j][1] = 150 + tmp;
					tx0[i][j][2] = 150 + tmp;
				}
			}
	}
}

void ShowText(char* text)
{
	glListBase(letters);
	glCallLists(1, GL_UNSIGNED_BYTE, text);
}

void initFont()
{
	HDC hdc = wglGetCurrentDC();

	HFONT hf;
	GLYPHMETRICSFLOAT gm[128];
	LOGFONT lf;

	lf.lfHeight = 25;
	lf.lfWidth = 0;
	lf.lfWeight = FW_NORMAL;
	lf.lfEscapement = 0;
	lf.lfItalic = true;
	lf.lfUnderline = false;
	lf.lfCharSet = DEFAULT_CHARSET; // can be hebrew
	strcpy((char*)lf.lfFaceName, "Arial");

	hf = CreateFontIndirect(&lf);
	SelectObject(hdc, hf);

	letters = glGenLists(128);
	wglUseFontOutlines(hdc, 0, 128, letters, 0, 0.2, WGL_FONT_POLYGONS, gm);
}

void drawBuilding(int x, int z)
{
	int numFloors = gridBuildingHeights[x][z];
	double buildingBaseHeight = ((ground[x - terrainCheckRadius][z - terrainCheckRadius] + ground[x - terrainCheckRadius][z + terrainCheckRadius] + ground[x + terrainCheckRadius][z + terrainCheckRadius] + ground[x + terrainCheckRadius][z - terrainCheckRadius]) / 4.0) + numFloors;

	// Draw walls and windows
	drawFacade(x - terrainCheckRadius, z - terrainCheckRadius, x - terrainCheckRadius, z + terrainCheckRadius, buildingBaseHeight, numFloors, -0.01);
	drawFacade(x - terrainCheckRadius, z - terrainCheckRadius, x + terrainCheckRadius, z - terrainCheckRadius, buildingBaseHeight, numFloors, -0.01);
	drawFacade(x + terrainCheckRadius, z - terrainCheckRadius, x + terrainCheckRadius, z + terrainCheckRadius, buildingBaseHeight, numFloors, 0.01);
	drawFacade(x - terrainCheckRadius, z + terrainCheckRadius, x + terrainCheckRadius, z + terrainCheckRadius, buildingBaseHeight, numFloors, 0.01);

	// Draw roof
	setRoofColor();
	drwaRoofForBuilding(x, z, buildingBaseHeight, 2);
}

void drawFacade(int baseX, int baseZ, int limitX, int limitZ, double facadeHeight, int totalFloors, double windowShift)
{
	setWallColor();
	drawWallForBuilding(baseX, baseZ, limitX, limitZ, facadeHeight);
	setWindowColor();
	drawWindowsForBuildings(baseX, baseZ, limitX, limitZ, facadeHeight, totalFloors, windowShift);
}

void setWallColor()
{
	glColor3d(1.0, 0.75, 0.45); // Light brown/beige color for walls
}

void setWindowColor()
{
	glColor3d(0.0, 0.0, 0.0); // Black for windows
}

void setRoofColor()
{
	glColor3d(0.5, 0.0, 0.13); // Strong burgundy color
}

void SimulateHydraulicErosion(int iterations, int raindrops_per_iteration, double erosion_factor, double min_water_volume)
{
	for (int iter = 0; iter < iterations; iter++)
	{
		for (int drop = 0; drop < raindrops_per_iteration; drop++)
		{
			int x = rand() % GSZ;
			int z = rand() % GSZ;
			double water_volume = 1.0; // Initial water volume

			double previous_height = ground[z][x];
			while (water_volume > min_water_volume) // Continue until water evaporates below threshold
			{
				// Calculate dynamic erosion based on height change
				double height_difference = previous_height - ground[z][x];
				double dynamic_erosion_factor = erosion_factor * (1 + height_difference);

				double erosion_amount = dynamic_erosion_factor * water_volume;
				ground[z][x] -= erosion_amount; // Erode the current spot
				previous_height = ground[z][x];

				// Find the lowest adjacent square
				int lowest_x = x, lowest_z = z;
				double lowest_height = ground[z][x];
				for (int dx = -1; dx <= 1; dx++)
				{
					for (int dz = -1; dz <= 1; dz++)
					{
						int nx = x + dx, nz = z + dz;
						if (nx >= 0 && nx < GSZ && nz >= 0 && nz < GSZ)
						{
							if (ground[nz][nx] < lowest_height)
							{
								lowest_height = ground[nz][nx];
								lowest_x = nx;
								lowest_z = nz;
							}
						}
					}
				}

				// Move the drop to the lowest adjacent square if it's lower than the current
				if (lowest_x == x && lowest_z == z)
				{
					break; // If no lower neighbor, stop moving this drop
				}

				x = lowest_x;
				z = lowest_z;
				water_volume *= 0.95; // Reduce volume to simulate water loss
			}
		}
	}
}

void drawWallForBuilding(int initialX, int initialZ, int finalX, int finalZ, double maxHeight)
{
	// Start drawing a polygon
	glBegin(GL_POLYGON);

	// Define vertices of the polygon
	glVertex3d(initialZ - GRID_HALF_SIZE, ground[initialX][initialZ], initialX - GRID_HALF_SIZE);
	glVertex3d(finalZ - GRID_HALF_SIZE, ground[finalX][finalZ], finalX - GRID_HALF_SIZE);
	glVertex3d(finalZ - GRID_HALF_SIZE, maxHeight, finalX - GRID_HALF_SIZE);
	glVertex3d(initialZ - GRID_HALF_SIZE, maxHeight, initialX - GRID_HALF_SIZE);

	// End drawing the polygon
	glEnd();
}

void drawWindow(double startX, double startZ, double endX, double endZ, double baseHeight, double topHeight) {
	glBegin(GL_POLYGON);
	glVertex3d(startZ - GRID_HALF_SIZE, baseHeight, startX - GRID_HALF_SIZE);
	glVertex3d(endZ - GRID_HALF_SIZE, baseHeight, endX - GRID_HALF_SIZE);
	glVertex3d(endZ - GRID_HALF_SIZE, topHeight, endX - GRID_HALF_SIZE);
	glVertex3d(startZ - GRID_HALF_SIZE, topHeight, startX - GRID_HALF_SIZE);
	glEnd();
}

void drawWindowsForBuildings(int baseX, int baseZ, int limitX, int limitZ, double peakHeight, int numFloors, double offset)
{
	int windowsPerFacade = 4;
	double facadeWidth = (limitZ - baseZ + limitX - baseX) / 2 + 1; // Simplified wall width calculation
	double windowWidth = facadeWidth / (windowsPerFacade * 2 + 1); // Calculate width of each window
	double baseHeight = (ground[baseX][baseZ] + ground[limitX][limitZ]) / 2;
	double heightIncrement = (peakHeight - baseHeight) / (double)(numFloors * 2);

	// Iterate over each floor level where windows will be placed
	for (int floor = 1; floor < numFloors * 2; floor += 2)
	{
		double windowBaseHeight = baseHeight + floor * heightIncrement;
		double windowTopHeight = windowBaseHeight + heightIncrement;

		// Process window placements for the current floor level
		for (int windowIndex = 1; windowIndex < windowsPerFacade * 2; windowIndex += 2)
		{
			if (windowTopHeight >= peakHeight)
			{
				continue;  // Skip iteration if the window's top height exceeds the peak height
			}

			double windowStartX, windowEndX, windowStartZ, windowEndZ;

			if (baseX == limitX)  // Vertical alignment
			{
				windowStartX = windowEndX = baseX + offset;
				windowStartZ = baseZ + windowIndex * windowWidth;
				windowEndZ = windowStartZ + windowWidth;
			}
			else if (baseZ == limitZ)  // Horizontal alignment
			{
				windowStartZ = windowEndZ = baseZ + offset;
				windowStartX = baseX + windowIndex * windowWidth;
				windowEndX = windowStartX + windowWidth;
			}

			// Invoke function to draw the window based on calculated coordinates
			drawWindow(windowStartX, windowStartZ, windowEndX, windowEndZ, windowBaseHeight, windowTopHeight);
		}

	}
}

void drwaRoofForBuilding(int posX, int posZ, double baseHeight, double peakIncrement)
{
	int radiusOffset = terrainCheckRadius; // The radius for checking terrain
	int halfGridSize = GSZ / 2;            // Half the grid size for adjustments

	// Points defining the roof's corners
	double corner1X = posX - radiusOffset - halfGridSize;
	double corner1Z = posZ - radiusOffset - halfGridSize;
	double corner2X = posX + radiusOffset - halfGridSize;
	double corner2Z = posZ + radiusOffset - halfGridSize;
	double peakX = posX - halfGridSize;
	double peakZ = posZ - halfGridSize;

	// Draw triangular roof sections
	glBegin(GL_POLYGON);
	glVertex3d(corner1Z, baseHeight, corner1X);
	glVertex3d(corner2Z, baseHeight, corner1X);
	glVertex3d(peakZ, baseHeight + peakIncrement, peakX);
	glEnd();

	glBegin(GL_POLYGON);
	glVertex3d(corner1Z, baseHeight, corner2X);
	glVertex3d(corner2Z, baseHeight, corner2X);
	glVertex3d(peakZ, baseHeight + peakIncrement, peakX);
	glEnd();

	glBegin(GL_POLYGON);
	glVertex3d(corner1Z, baseHeight, corner1X);
	glVertex3d(corner1Z, baseHeight, corner2X);
	glVertex3d(peakZ, baseHeight + peakIncrement, peakX);
	glEnd();

	glBegin(GL_POLYGON);
	glVertex3d(corner2Z, baseHeight, corner1X);
	glVertex3d(corner2Z, baseHeight, corner2X);
	glVertex3d(peakZ, baseHeight + peakIncrement, peakX);
	glEnd();
}
