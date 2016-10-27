//Code originally written by Richard O. Lee
//Modified by Christian Bienia and Christian Fensch

#include <cstdlib>
#include <cstring>

#include <iostream>
#include <fstream>
#include <math.h>
#include <stdint.h>
#include <assert.h>

#ifdef ENABLE_PARSEC_HOOKS
#include <hooks.h>
#endif

static inline int isLittleEndian() {
  union {
    uint16_t word;
    uint8_t byte;
  } endian_test;

  endian_test.word = 0x00FF;
  return (endian_test.byte == 0xFF);
}

union __float_and_int {
  uint32_t i;
  float    f;
};

static inline float bswap_float(float x) {
  union __float_and_int __x;

   __x.f = x;
   __x.i = ((__x.i & 0xff000000) >> 24) | ((__x.i & 0x00ff0000) >>  8) |
           ((__x.i & 0x0000ff00) <<  8) | ((__x.i & 0x000000ff) << 24);

  return __x.f;
}

static inline int bswap_int32(int x) {
  return ( (((x) & 0xff000000) >> 24) | (((x) & 0x00ff0000) >>  8) |
           (((x) & 0x0000ff00) <<  8) | (((x) & 0x000000ff) << 24) );
}

////////////////////////////////////////////////////////////////////////////////

// note: icc-optimized version of this class gave 15% more
// performance than our hand-optimized SSE3 implementation
class Vec3
{
public:
    float x, y, z;

    Vec3() {}
    Vec3(float _x, float _y, float _z) : x(_x), y(_y), z(_z) {}

    float   GetLengthSq() const         { return x*x + y*y + z*z; }
    float   GetLength() const           { return sqrtf(GetLengthSq()); }
    Vec3 &  Normalize()                 { return *this /= GetLength(); }

    Vec3 &  operator += (Vec3 const &v) { x += v.x;  y += v.y; z += v.z; return *this; }
    Vec3 &  operator -= (Vec3 const &v) { x -= v.x;  y -= v.y; z -= v.z; return *this; }
    Vec3 &  operator *= (float s)       { x *= s;  y *= s; z *= s; return *this; }
    Vec3 &  operator /= (float s)       { x /= s;  y /= s; z /= s; return *this; }

    Vec3    operator + (Vec3 const &v) const    { return Vec3(x+v.x, y+v.y, z+v.z); }
    Vec3    operator - () const                 { return Vec3(-x, -y, -z); }
    Vec3    operator - (Vec3 const &v) const    { return Vec3(x-v.x, y-v.y, z-v.z); }
    Vec3    operator * (float s) const          { return Vec3(x*s, y*s, z*s); }
    Vec3    operator / (float s) const          { return Vec3(x/s, y/s, z/s); }
	
    float   operator * (Vec3 const &v) const    { return x*v.x + y*v.y + z*v.z; }
};

////////////////////////////////////////////////////////////////////////////////

// there is a current limitation of 16 particles per cell
// (this structure use to be a simple linked-list of particles but, due to
// improved cache locality, we get a huge performance increase by copying
// particles instead of referencing them)
struct Cell
{
	Vec3 p[16];
	Vec3 hv[16];
	Vec3 v[16];
	Vec3 a[16];
	float density[16];
};

////////////////////////////////////////////////////////////////////////////////

const float timeStep = 0.005f;
const float doubleRestDensity = 2000.f;
const float kernelRadiusMultiplier = 1.695f;
const float stiffness = 1.5f;
const float viscosity = 0.4f;
const Vec3 externalAcceleration(0.f, -9.8f, 0.f);
const Vec3 domainMin(-0.065f, -0.08f, -0.065f);
const Vec3 domainMax(0.065f, 0.1f, 0.065f);

float restParticlesPerMeter, h, hSq;
float densityCoeff, pressureCoeff, viscosityCoeff;

int nx, ny, nz;     // number of grid cells in each dimension
Vec3 delta;         // cell dimensions
int origNumParticles = 0;
int numParticles = 0;
int numCells = 0;
Cell *cells = 0;
Cell *cells2 = 0;
int *cnumPars = 0;
int *cnumPars2 = 0;

////////////////////////////////////////////////////////////////////////////////

void InitSim(char const *fileName)
{
	std::cout << "Loading file \"" << fileName << "\"..." << std::endl;
	std::ifstream file(fileName, std::ios::binary);
	assert(file);

	file.read((char *)&restParticlesPerMeter, 4);
	file.read((char *)&origNumParticles, 4);
        if(!isLittleEndian()) {
          restParticlesPerMeter = bswap_float(restParticlesPerMeter);
          origNumParticles      = bswap_int32(origNumParticles);
        }
	numParticles = origNumParticles;

	h = kernelRadiusMultiplier / restParticlesPerMeter;
	hSq = h*h;
	const float pi = 3.14159265358979f;
	float coeff1 = 315.f / (64.f*pi*pow(h,9.f));
	float coeff2 = 15.f / (pi*pow(h,6.f));
	float coeff3 = 45.f / (pi*pow(h,6.f));
	float particleMass = 0.5f*doubleRestDensity / (restParticlesPerMeter*restParticlesPerMeter*restParticlesPerMeter);
	densityCoeff = particleMass * coeff1;
	pressureCoeff = 3.f*coeff2 * 0.5f*stiffness * particleMass;
	viscosityCoeff = viscosity * coeff3 * particleMass;

	Vec3 range = domainMax - domainMin;
	nx = (int)(range.x / h);
	ny = (int)(range.y / h);
	nz = (int)(range.z / h);
	assert(nx >= 1 && ny >= 1 && nz >= 1);
	numCells = nx*ny*nz;
	std::cout << "Number of cells: " << numCells << std::endl;
	delta.x = range.x / nx;
	delta.y = range.y / ny;
	delta.z = range.z / nz;
	assert(delta.x >= h && delta.y >= h && delta.z >= h);

	cells = new Cell[numCells];
	cells2 = new Cell[numCells];
	cnumPars = new int[numCells];
	cnumPars2 = new int[numCells];
	assert(cells && cells2 && cnumPars && cnumPars2);

	memset(cnumPars2, 0, numCells*sizeof(int));

	float px, py, pz, hvx, hvy, hvz, vx, vy, vz;
	for(int i = 0; i < origNumParticles; ++i)
	{
		file.read((char *)&px, 4);
		file.read((char *)&py, 4);
		file.read((char *)&pz, 4);
		file.read((char *)&hvx, 4);
		file.read((char *)&hvy, 4);
		file.read((char *)&hvz, 4);
		file.read((char *)&vx, 4);
		file.read((char *)&vy, 4);
		file.read((char *)&vz, 4);
                if(!isLittleEndian()) {
                  px  = bswap_float(px);
                  py  = bswap_float(py);
                  pz  = bswap_float(pz);
                  hvx = bswap_float(hvx);
                  hvy = bswap_float(hvy);
                  hvz = bswap_float(hvz);
                  vx  = bswap_float(vx);
                  vy  = bswap_float(vy);
                  vz  = bswap_float(vz);
                }

		int ci = (int)((px - domainMin.x) / delta.x);
		int cj = (int)((py - domainMin.y) / delta.y);
		int ck = (int)((pz - domainMin.z) / delta.z);

		if(ci < 0) ci = 0; else if(ci > (nx-1)) ci = nx-1;
		if(cj < 0) cj = 0; else if(cj > (ny-1)) cj = ny-1;
		if(ck < 0) ck = 0; else if(ck > (nz-1)) ck = nz-1;

		int index = (ck*ny + cj)*nx + ci;
		Cell &cell = cells2[index];

		int np = cnumPars2[index];
		if(np < 16)
		{
			cell.p[np].x = px;
			cell.p[np].y = py;
			cell.p[np].z = pz;
			cell.hv[np].x = hvx;
			cell.hv[np].y = hvy;
			cell.hv[np].z = hvz;
			cell.v[np].x = vx;
			cell.v[np].y = vy;
			cell.v[np].z = vz;
			++cnumPars2[index];
		}
		else
			--numParticles;
	}

	std::cout << "Number of particles: " << numParticles << " (" << origNumParticles-numParticles << " skipped)" << std::endl;
}

////////////////////////////////////////////////////////////////////////////////

void SaveFile(char const *fileName)
{
	std::cout << "Saving file \"" << fileName << "\"..." << std::endl;

	std::ofstream file(fileName, std::ios::binary);
	assert(file);

        if(!isLittleEndian()) {
          float restParticlesPerMeter_le;
          int   origNumParticles_le;

          restParticlesPerMeter_le = bswap_float(restParticlesPerMeter);
          origNumParticles_le      = bswap_int32(origNumParticles);
	  file.write((char *)&restParticlesPerMeter_le, 4);
	  file.write((char *)&origNumParticles_le,      4);
        } else {
	  file.write((char *)&restParticlesPerMeter, 4);
	  file.write((char *)&origNumParticles, 4);
        }

	int count = 0;
	for(int i = 0; i < numCells; ++i)
	{
		Cell const &cell = cells[i];
		int np = cnumPars[i];
		for(int j = 0; j < np; ++j)
		{
                        if(!isLittleEndian()) {
                          float px, py, pz, hvx, hvy, hvz, vx,vy, vz;

                          px  = bswap_float(cell.p[j].x);
                          py  = bswap_float(cell.p[j].y);
                          pz  = bswap_float(cell.p[j].z);
                          hvx = bswap_float(cell.hv[j].x);
                          hvy = bswap_float(cell.hv[j].y);
                          hvz = bswap_float(cell.hv[j].z);
                          vx  = bswap_float(cell.v[j].x);
                          vy  = bswap_float(cell.v[j].y);
                          vz  = bswap_float(cell.v[j].z);

			  file.write((char *)&px,  4);
			  file.write((char *)&py,  4);
			  file.write((char *)&pz,  4);
			  file.write((char *)&hvx, 4);
			  file.write((char *)&hvy, 4);
			  file.write((char *)&hvz, 4);
			  file.write((char *)&vx,  4);
			  file.write((char *)&vy,  4);
			  file.write((char *)&vz,  4);
                        } else {
			  file.write((char *)&cell.p[j].x,  4);
			  file.write((char *)&cell.p[j].y,  4);
			  file.write((char *)&cell.p[j].z,  4);
			  file.write((char *)&cell.hv[j].x, 4);
			  file.write((char *)&cell.hv[j].y, 4);
			  file.write((char *)&cell.hv[j].z, 4);
			  file.write((char *)&cell.v[j].x,  4);
			  file.write((char *)&cell.v[j].y,  4);
			  file.write((char *)&cell.v[j].z,  4);
                        }
			++count;
		}
	}
	assert(count == numParticles);

	int numSkipped = origNumParticles - numParticles;
	float zero = 0.f;
        if(!isLittleEndian()) {
          zero = bswap_float(zero);
        }
	for(int i = 0; i < numSkipped; ++i)
	{
		file.write((char *)&zero, 4);
		file.write((char *)&zero, 4);
		file.write((char *)&zero, 4);
		file.write((char *)&zero, 4);
		file.write((char *)&zero, 4);
		file.write((char *)&zero, 4);
		file.write((char *)&zero, 4);
		file.write((char *)&zero, 4);
		file.write((char *)&zero, 4);
	}
}

////////////////////////////////////////////////////////////////////////////////

void CleanUpSim()
{
	delete[] cells;
    delete[] cells2;
	delete[] cnumPars;
	delete[] cnumPars2;
}

////////////////////////////////////////////////////////////////////////////////

void RebuildGrid()
{
	memset(cnumPars, 0, numCells*sizeof(int));

	for(int i = 0; i < numCells; ++i)
	{
		Cell const &cell2 = cells2[i];
		int np = cnumPars2[i];
		for(int j = 0; j < np; ++j)
		{
			int ci = (int)((cell2.p[j].x - domainMin.x) / delta.x);
			int cj = (int)((cell2.p[j].y - domainMin.y) / delta.y);
			int ck = (int)((cell2.p[j].z - domainMin.z) / delta.z);

			if(ci < 0) ci = 0; else if(ci > (nx-1)) ci = nx-1;
			if(cj < 0) cj = 0; else if(cj > (ny-1)) cj = ny-1;
			if(ck < 0) ck = 0; else if(ck > (nz-1)) ck = nz-1;

			int index = (ck*ny + cj)*nx + ci;
			Cell &cell = cells[index];

			int np2 = cnumPars[index];
			cell.p[np2].x = cell2.p[j].x;
			cell.p[np2].y = cell2.p[j].y;
			cell.p[np2].z = cell2.p[j].z;
			cell.hv[np2].x = cell2.hv[j].x;
			cell.hv[np2].y = cell2.hv[j].y;
			cell.hv[np2].z = cell2.hv[j].z;
			cell.v[np2].x = cell2.v[j].x;
			cell.v[np2].y = cell2.v[j].y;
			cell.v[np2].z = cell2.v[j].z;

			++cnumPars[index];
		}
	}
}

////////////////////////////////////////////////////////////////////////////////

int GetNeighborCells(int ci, int cj, int ck, int *neighCells)
{
	int numNeighCells = 0;

	for(int di = -1; di <= 1; ++di)
		for(int dj = -1; dj <= 1; ++dj)
			for(int dk = -1; dk <= 1; ++dk)
			{
				int ii = ci + di;
				int jj = cj + dj;
				int kk = ck + dk;
				if(ii >= 0 && ii < nx && jj >= 0 && jj < ny && kk >= 0 && kk < nz)
				{
					int index = (kk*ny + jj)*nx + ii;
					if(cnumPars[index] != 0)
					{
						neighCells[numNeighCells] = index;
						++numNeighCells;
					}
				}
			}

	return numNeighCells;
}

////////////////////////////////////////////////////////////////////////////////

void ComputeForces()
{
	for(int i = 0; i < numCells; ++i)
	{
		Cell &cell = cells[i];
		int np = cnumPars[i];
		for(int j = 0; j < np; ++j)
		{
			cell.density[j] = 0.f;
			cell.a[j] = externalAcceleration;
		}
	}

	int neighCells[27];

	int cindex = 0;
	for(int ck = 0; ck < nz; ++ck)
		for(int cj = 0; cj < ny; ++cj)
			for(int ci = 0; ci < nx; ++ci, ++cindex)
			{
				int numPars = cnumPars[cindex];
				if(numPars == 0)
					continue;

				int numNeighCells = GetNeighborCells(ci, cj, ck, neighCells);

				Cell &cell = cells[cindex];

				for(int ipar = 0; ipar < numPars; ++ipar)
					for(int inc = 0; inc < numNeighCells; ++inc)
					{
						int cindexNeigh = neighCells[inc];
						Cell &neigh = cells[cindexNeigh];
						int numNeighPars = cnumPars[cindexNeigh];
						for(int iparNeigh = 0; iparNeigh < numNeighPars; ++iparNeigh)
							if(&neigh.p[iparNeigh] < &cell.p[ipar])
							{
								float distSq = (cell.p[ipar] - neigh.p[iparNeigh]).GetLengthSq();
								if(distSq < hSq)
								{
									float t = hSq - distSq;
									float tc = t*t*t;
									cell.density[ipar] += tc;
									neigh.density[iparNeigh] += tc;
								}
							}
					}
			}

	const float tc = hSq*hSq*hSq;
	for(int i = 0; i < numCells; ++i)
	{
		Cell &cell = cells[i];
		int np = cnumPars[i];
		for(int j = 0; j < np; ++j)
		{
			cell.density[j] += tc;
			cell.density[j] *= densityCoeff;
		}
	}

	cindex = 0;
	for(int ck = 0; ck < nz; ++ck)
		for(int cj = 0; cj < ny; ++cj)
			for(int ci = 0; ci < nx; ++ci, ++cindex)
			{
				int numPars = cnumPars[cindex];
				if(numPars == 0)
					continue;

				int numNeighCells = GetNeighborCells(ci, cj, ck, neighCells);

				Cell &cell = cells[cindex];

				for(int ipar = 0; ipar < numPars; ++ipar)
					for(int inc = 0; inc < numNeighCells; ++inc)
					{
						int cindexNeigh = neighCells[inc];
						Cell &neigh = cells[cindexNeigh];
						int numNeighPars = cnumPars[cindexNeigh];
						for(int iparNeigh = 0; iparNeigh < numNeighPars; ++iparNeigh)
							if(&neigh.p[iparNeigh] < &cell.p[ipar])
							{
								Vec3 disp = cell.p[ipar] - neigh.p[iparNeigh];
								float distSq = disp.GetLengthSq();
								if(distSq < hSq)
								{
									float dist = sqrtf(std::max(distSq, 1e-12f));
									float hmr = h - dist;

									Vec3 acc = disp * pressureCoeff * (hmr*hmr/dist) * (cell.density[ipar]+neigh.density[iparNeigh] - doubleRestDensity);
									acc += (neigh.v[iparNeigh] - cell.v[ipar]) * viscosityCoeff * hmr;
									acc /= cell.density[ipar] * neigh.density[iparNeigh];

									cell.a[ipar] += acc;
									neigh.a[iparNeigh] -= acc;
								}
							}
					}
			}
}

////////////////////////////////////////////////////////////////////////////////

void ProcessCollisions()
{
	const float parSize = 0.0002f;
    const float epsilon = 1e-10f;
    const float stiffness = 30000.f;
    const float damping = 128.f;

	for(int i = 0; i < numCells; ++i)
	{
		Cell &cell = cells[i];
		int np = cnumPars[i];
		for(int j = 0; j < np; ++j)
		{
			Vec3 pos = cell.p[j] + cell.hv[j] * timeStep;

			float diff = parSize - (pos.x - domainMin.x);
			if(diff > epsilon)
				cell.a[j].x += stiffness*diff - damping*cell.v[j].x;

			diff = parSize - (domainMax.x - pos.x);
			if(diff > epsilon)
				cell.a[j].x -= stiffness*diff + damping*cell.v[j].x;

			diff = parSize - (pos.y - domainMin.y);
			if(diff > epsilon)
				cell.a[j].y += stiffness*diff - damping*cell.v[j].y;

			diff = parSize - (domainMax.y - pos.y);
			if(diff > epsilon)
				cell.a[j].y -= stiffness*diff + damping*cell.v[j].y;

			diff = parSize - (pos.z - domainMin.z);
			if(diff > epsilon)
				cell.a[j].z += stiffness*diff - damping*cell.v[j].z;

			diff = parSize - (domainMax.z - pos.z);
			if(diff > epsilon)
				cell.a[j].z -= stiffness*diff + damping*cell.v[j].z;
		}
	}
}

////////////////////////////////////////////////////////////////////////////////

void AdvanceParticles()
{
	for(int i = 0; i < numCells; ++i)
	{
		Cell &cell = cells[i];
		int np = cnumPars[i];
		for(int j = 0; j < np; ++j)
		{
			Vec3 v_half = cell.hv[j] + cell.a[j]*timeStep;
			cell.p[j] += v_half * timeStep;
			cell.v[j] = cell.hv[j] + v_half;
			cell.v[j] *= 0.5f;
			cell.hv[j] = v_half;
		}
	}
}

////////////////////////////////////////////////////////////////////////////////

void AdvanceFrame()
{
    RebuildGrid();
	ComputeForces();
	ProcessCollisions();
	AdvanceParticles();
}

////////////////////////////////////////////////////////////////////////////////

int main(int argc, char *argv[])
{
#ifdef PARSEC_VERSION
#define __PARSEC_STRING(x) #x
#define __PARSEC_XSTRING(x) __PARSEC_STRING(x)
        std::cout << "PARSEC Benchmark Suite Version "__PARSEC_XSTRING(PARSEC_VERSION) << std::endl << std::flush;
#else
        std::cout << "PARSEC Benchmark Suite" << std::endl << std::flush;
#endif //PARSEC_VERSION
#ifdef ENABLE_PARSEC_HOOKS
	__parsec_bench_begin(__parsec_fluidanimate);
#endif

	if(argc < 4 || argc >= 6)
	{
		std::cout << "Usage: " << argv[0] << " <threadnum> <framenum> <.fluid input file> [.fluid output file]" << std::endl;
		return -1;
	}

	int threadnum = atoi(argv[1]);
	int framenum = atoi(argv[2]);

	//Check arguments
	if(threadnum != 1) {
		std::cerr << "<threadnum> must be 1 (serial version)" << std::endl;
		return -1;
	}
	if(framenum < 1) {
		std::cerr << "<framenum> must at least be 1" << std::endl;
		return -1;
	}

	InitSim(argv[3]);

#ifdef ENABLE_PARSEC_HOOKS
	__parsec_roi_begin();
#endif
	for(int i = 0; i < framenum; ++i)
		AdvanceFrame();
#ifdef ENABLE_PARSEC_HOOKS
	__parsec_roi_end();
#endif

	if(argc > 4)
		SaveFile(argv[4]);

	CleanUpSim();

#ifdef ENABLE_PARSEC_HOOKS
	__parsec_bench_end();
#endif

	return 0;
}

////////////////////////////////////////////////////////////////////////////////

