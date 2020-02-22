#pragma once


struct gline {
	double X1, Y1, Z1;
	double X2, Y2, Z2;

	double toolradius;
	double toolangle;
	int tool;

	double minX, maxX, minY, maxY;
};
