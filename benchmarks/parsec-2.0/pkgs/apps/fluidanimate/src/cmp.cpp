#include <cstdlib>

#include <iostream>
#include <fstream>
#include <math.h>

using namespace std;

////////////////////////////////////////////////////////////////////////////////

int main(int argc, char *argv[])
{
	if(argc < 3)
	{
		cout << "Usage: " << argv[0] << " <.fluid file> <.fluid file> [tol=0.0001]" << endl;
		return -1;
	}

	float tol = 0.00001f;
	if(argc > 3)
		tol = atof(argv[3]);
	cout << "Tolerance: " << tol << endl;

	ifstream file1(argv[1], ios::binary);
	ifstream file2(argv[2], ios::binary);
	if(!file1 || !file2)
	{
		cout << "Could not open file." << endl;
		return -1;
	}

	int rppm1, rppm2;
	file1.read((char *)&rppm1, 4);
	file2.read((char *)&rppm2, 4);
	if(rppm1 != rppm2)
	{
		cout << "Rest particles per meter values differ (" << rppm1 << " vs. " << rppm2 << ")." << endl;
		return 0;
	}

	int np1, np2;
	file1.read((char *)&np1, 4);
	file2.read((char *)&np2, 4);
	if(np1 != np2)
	{
		cout << "Number of particles values differ (" << np1 << " vs. " << np2 << ")." << endl;
		return 0;
	}

	for(int i = 0; i < np1*9; ++i)
	{
		float v1, v2;
		file1.read((char *)&v1, 4);
		file2.read((char *)&v2, 4);
		bool pos = i%9 < 3;
		if(pos && fabsf(v1-v2) > tol)
		{
			cout << i << ": Values differ (" << v1 << " vs. " << v2 << ")." << endl;
			return 0;
		}
	}

	cout << "Files match." << std::endl;

	return 0;
}

////////////////////////////////////////////////////////////////////////////////

