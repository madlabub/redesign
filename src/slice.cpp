#include "slice.hpp"

// Constructors
Slice::Slice()
{
	contour.clear();
	setScale(100);
}

Slice::Slice(const int scale)
{
	contour.clear();
	setScale(scale);
}

Slice::Slice(const string& filename)
{
	contour.clear();
	setScale(100);
	LoadFromFile(filename);
}

Slice::Slice(const string& filename, const int scale)
{
	contour.clear();
	setScale(scale);
	LoadFromFile(filename);
}

// stores the scale (for precision)
void Slice::setScale(const int value)
{
	scale = value;
}

// sets the contour to specified vector of polygons
void Slice::setContour(Paths ppg)
{
	contour = ppg;
}

// returns the contour
Paths Slice::getContour()
{
	return contour;
}

// returns a vector with the perimeter of each polygon in the contour
std::vector<double> Slice::getContourLengths()
{
	std::vector<double> dists;
	for (size_t i = 0; i < contour.size(); ++i)
	{
		dists.push_back(0.0);
		for (size_t j = 0; j < contour[i].size(); ++j)
		{
			dists[i] += getDistance(contour[i][j], contour[i][(j + 1) % contour[i].size()]);
		}
	}
	return dists;
}

// resamples the contour with N equi-distant points
Paths Slice::resampleContour(int N)
{
	std::vector<double> dists = getContourLengths();
	double total_dist = 0.0;
	Paths ppg;
	// total the perimeter of every loop
	for (size_t i = 0; i < dists.size(); ++i)
		total_dist += dists[i];
	std::vector<int> numPoints;
	for (size_t i = 0; i < dists.size(); ++i)
	{
		// distribute the number of samples based on perimeter of each loop and then sample them
		numPoints.push_back(N * (int)round(dists[i] / total_dist));
		ppg.push_back(resamplePath(contour[i], numPoints[i], dists[i]));
	}
	return ppg;
}

// returns the unit normal (diagonal) at one vertex
Vector2<double> Slice::getNormalAt(int vertexId, int loopId)
{
	Vector2<double> vec1, vec2, normal;
	Path pg = contour[loopId];
	int sz = (int)pg.size();
	vec1.x = (double)(pg[(vertexId + 1) % sz].X - pg[vertexId % sz].X);
	vec1.y = (double)(pg[(vertexId + 1) % sz].Y - pg[vertexId % sz].Y);
	vec2.x = (double)(pg[vertexId % sz].X - pg[(vertexId - 1) % sz].X);
	vec2.y = (double)(pg[vertexId % sz].Y - pg[(vertexId - 1) % sz].Y);
	normal = vec1 - vec2;
	normal.normalize();
	return normal;
}

// returns the unit normal at all vertices in a loop
std::vector<Vector2<double>> Slice::getLoopNormals(int loopId)
{
	Vector2<double> vec1, vec2, normal;
	std::vector<Vector2<double>> normal_vec;
	Path pg = contour[loopId];
	int sz = (int)pg.size();
	vec2.x = (double)(pg[0].X - pg[sz - 1].X);
	vec2.y = (double)(pg[0].Y - pg[sz - 1].Y);

	for (size_t i = 0; i < sz - 1; i++)
	{
		vec1.x = (double)(pg[i + 1].X - pg[i].X);
		vec1.y = (double)(pg[i + 1].Y - pg[i].Y);
		normal = vec1 - vec2;
		normal.normalize();
		normal_vec.push_back(normal);
		vec2 = vec1;
	}
	return normal_vec;
}

// returns the unit normal at all vertices in the slice
std::vector<std::vector<Vector2<double>>> Slice::getAllNormals()
{
	std::vector<std::vector<Vector2<double>>> normals;
	for (size_t i = 0; i < contour.size(); i++)
		normals.push_back(getLoopNormals((int)i));
	return normals;
}

// saves the contour in a file
bool Slice::SaveToFile(const string& filename, unsigned decimal_places)
{
	ofstream ofs(filename);
	if (!ofs) return false;

	if (decimal_places > 8) decimal_places = 8;
	ofs << setprecision(decimal_places) << std::fixed;

	Path pg;
	for (size_t i = 0; i < contour.size(); ++i)
	{
		for (size_t j = 0; j < contour[i].size(); ++j)
			ofs << (double)contour[i][j].X / scale << ", " << (double)contour[i][j].Y / scale << "," << std::endl;
		ofs << std::endl;
	}
	ofs.close();
	return true;
}

// reads in the contour from a file
bool Slice::LoadFromFile(const string& filename)
{
	//file format assumes: 
	//  1. path coordinates (x,y) are comma separated (+/- spaces) and 
	//  each coordinate is on a separate line
	//  2. each path is separated by one or more blank lines

	contour.clear();
	ifstream ifs(filename);
	if (!ifs) return false;
	string line;
	Path pg;
	while (std::getline(ifs, line))
	{
		stringstream ss(line);
		double X = 0.0, Y = 0.0;
		if (!(ss >> X))
		{
			//ie blank lines => flag start of next polygon 
			if (pg.size() > 0) contour.push_back(pg);
			pg.clear();
			continue;
		}
		char c = ss.peek();
		while (c == ' ') { ss.read(&c, 1); c = ss.peek(); } //gobble spaces before comma
		if (c == ',') { ss.read(&c, 1); c = ss.peek(); } //gobble comma
		while (c == ' ') { ss.read(&c, 1); c = ss.peek(); } //gobble spaces after comma
		if (!(ss >> Y)) break; //oops!
		pg.push_back(IntPoint((cInt)(X * scale), (cInt)(Y * scale)));
	}
	if (pg.size() > 0) contour.push_back(pg);
	ifs.close();
	return true;
}

// main code

// get distance between two points
double getDistance(const IntPoint start, const IntPoint end)
{
	return sqrt(pow((start.X - end.X), 2) + pow((start.Y - end.Y), 2));
}

// makes a polygon by randomly selecting vertices inside a rectangle
void MakeRandomPoly(int edgeCount, int width, int height, int scale, Paths & poly)
{
	// reinitialize the vector
	poly.resize(1);
	poly[0].resize(edgeCount);
	// scale the bounding rectangle
	width *= scale;
	height *= scale;
	// loop to sample vertices insize the rectangle sequentially
	for (int i = 0; i < edgeCount; i++)
	{
		poly[0][i].X = rand() % width;
		poly[0][i].Y = rand() % height;
	}
}

// resamples a polygon with N equidistant points
Path resamplePath(Path& pg, int N, double pathlength)
{
	// compute pathlength if not already computed and passed in argument
	if (pathlength < 0)
	{
		pathlength = 0.0;
		for (size_t i = 0; i < pg.size(); ++i)
			pathlength += getDistance(pg[i], pg[(i + 1) % pg.size()]);
	}

	// initialize temporary holder and result holder
	Path temp = pg, rpg;
	temp.push_back(pg[0]); // insert the first point at the end since it's a loop
	rpg.push_back(temp[0]); // insert the first point in the result

	// initialize some variables
	int current = 0, next = 1, sz = (int)temp.size();
	double dist = 0.0, edgelength = 0.0, delta = pathlength / N;

	// loop until reaching end of loop or getting required number of samples
	// (both should ideally happen simultaneously)
	while (current < sz - 1 && rpg.size() < N)
	{
		edgelength = getDistance(temp[current], temp[next]); // get the current edge length
		if (edgelength + dist < delta)
		{
			// next vertex too close, increment distance and jump to next edge
			dist += edgelength;
		}
		else
		{
			// resample a new point at a distance (delta - distance covered) from current on the edge <current, next>
			IntPoint pnt(cInt(temp[current].X + (delta - dist) * (temp[next].X - temp[current].X) / edgelength),
				cInt(temp[current].Y + (delta - dist) * (temp[next].Y - temp[current].Y) / edgelength));


			// insert the new point in the result and at the next position and increment size of loop
			rpg.push_back(pnt);
			temp.insert(temp.begin() + next, pnt);
			sz++;
			dist = 0.0; // reinitialize distance
		}

		current = next++; // increment current and next
	}
	return rpg;
}

int main()
{
	Paths ppg;
	Slice s, rs;
	s.setScale(10000);
	rs.setScale(10000);
	MakeRandomPoly(10, 1, 1, 10000, ppg);
	s.setContour(ppg);
	s.SaveToFile("random_slice.txt", 3);


	rs.setContour(s.resampleContour(1000));
	rs.SaveToFile("resampled_random_slice.txt", 3);

	rs.getAllNormals();
	rs.getLoopNormals(0);
	rs.getNormalAt(1, 0);

	SVGBuilder svg;
	svg.style.penWidth = 0.8;
	svg.style.pft = pftEvenOdd;
	svg.style.brushClr = 0x1200009C;
	svg.style.penClr = 0xCCD3D3DA;
	svg.AddPaths(s.getContour());
	svg.style.brushClr = 0x6080ff9C;
	svg.style.penClr = 0xFF003300;
	svg.AddPaths(rs.getContour());
	svg.SaveToFile("solution.svg", 0.1);

	return 0;
}