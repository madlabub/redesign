#ifndef SLICE_HPP_INCLUDED
#define SLICE_HPP_INCLUDED

#include <iostream>
#include "clipper.hpp"
#include "SVGBuilder.hpp"

using namespace std;
using namespace ClipperLib;

class Slice
{
private:
	Paths contour;
	int scale;

public:
	Slice();
	Slice(const int scale);
	Slice(const string& filename);
	Slice(const string& filename, const int scale);
	void setScale(const int value);
	void setContour(Paths ppg);
	Paths getContour();
	std::vector<double> getContourLengths();
	Paths resampleContour(int N);
	bool LoadFromFile(const string& filename);
	bool SaveToFile(const string& filename, unsigned decimal_places = 0);
};

double getDistance(const IntPoint start, const IntPoint end);
Path resamplePath(Path& pg, int N = 100, double pathlength = -1.0);

#endif