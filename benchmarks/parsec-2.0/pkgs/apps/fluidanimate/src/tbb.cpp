//Code originally written by Richard O. Lee and Gilberto Contreras
//Modified by Christian Bienia and Christian Fensch
 
#include <cstdlib>
#include <cstring>

#include <iostream>
#include <fstream>
#include <math.h>
#include <stdint.h>
#include <pthread.h>
#include <assert.h>


#ifdef TBB_VERSION
int NUM_TBB_GRIDS;
#define NUM_TASKS     (8)
#include "tbb/blocked_range.h"
#include "tbb/parallel_reduce.h"
#include "tbb/task_scheduler_init.h"
#endif


//#define PRINTINFO


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

int nx, ny, nz;			// number of grid cells in each dimension
Vec3 delta;				// cell dimensions
int origNumParticles = 0;
int numParticles = 0;
int numCells = 0;
Cell *cells = 0;
Cell *cells2 = 0;
int *cnumPars = 0;
int *cnumPars2 = 0;

int XDIVS = 1;	// number of partitions in X
int ZDIVS = 1;	// number of partitions in Z

#define NUM_GRIDS  ((XDIVS) * (ZDIVS))

struct Grid
{
	int sx, sy, sz;
	int ex, ey, ez;
} *grids;
bool *border;			// flags which cells lie on grid boundaries
pthread_attr_t attr;
pthread_t *thread;
pthread_mutex_t **mutex;	// used to lock cells in RebuildGrid and also particles in other functions

////////////////////////////////////////////////////////////////////////////////

/*
 * hmgweight
 *
 * Computes the hamming weight of x
 *
 * x      - input value
 * lsb    - if x!=0 position of smallest bit set, else -1
 *
 * return - the hamming weight
 */
unsigned int hmgweight(unsigned int x, int *lsb) {
  unsigned int weight=0;
  unsigned int mask= 1;
  unsigned int count=0;
 
  *lsb=-1;
  while(x > 0) {
    unsigned int temp;
    temp=(x&mask);
    if((x&mask) == 1) {
      weight++;
      if(*lsb == -1) *lsb = count;
    }
    x >>= 1;
    count++;
  }

  return weight;
}

void InitSim(char const *fileName, unsigned int threadnum)
{
        //Compute partitioning based on square root of number of threads
        //NOTE: Other partition sizes are possible as long as XDIVS * ZDIVS == threadnum,
        //      but communication is minimal (and hence optimal) if XDIVS == ZDIVS
        int lsb;
        if(hmgweight(threadnum,&lsb) != 1) {
                std::cerr << "Number of threads must be a power of 2" << std::endl;
                exit(1);
        }
        XDIVS = 1<<(lsb/2);
        ZDIVS = 1<<(lsb/2);
        if(XDIVS*ZDIVS != threadnum) XDIVS*=2;
        assert(XDIVS * ZDIVS == threadnum);

        FILE *fp;
	thread = new pthread_t[NUM_GRIDS];
	grids = new struct Grid[NUM_GRIDS];

	fprintf(stderr,"Loading file \"%s\"\n",fileName);

	fp = fopen(fileName,"rb");
	assert(fp);
	
	fread((void *)&restParticlesPerMeter, sizeof(float), 1, fp);
	fread((void *)&origNumParticles, sizeof(int), 1, fp);
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

	assert(nx >= XDIVS && nz >= ZDIVS);
	int gi = 0;
	int sx, sz, ex, ez;
	ex = 0;
	for(int i = 0; i < XDIVS; ++i)
	{
		sx = ex;
		ex = int(float(nx)/float(XDIVS) * (i+1) + 0.5f);
		assert(sx < ex);

		ez = 0;
		for(int j = 0; j < ZDIVS; ++j, ++gi)
		{
			sz = ez;
			ez = int(float(nz)/float(ZDIVS) * (j+1) + 0.5f);
			assert(sz < ez);

			grids[gi].sx = sx;
			grids[gi].ex = ex;
			grids[gi].sy = 0;
			grids[gi].ey = ny;
			grids[gi].sz = sz;
			grids[gi].ez = ez;
		}
	}
	assert(gi == NUM_GRIDS);

	border = new bool[numCells];
	for(int i = 0; i < NUM_GRIDS; ++i)
		for(int iz = grids[i].sz; iz < grids[i].ez; ++iz)
			for(int iy = grids[i].sy; iy < grids[i].ey; ++iy)
				for(int ix = grids[i].sx; ix < grids[i].ex; ++ix)
				{
					int index = (iz*ny + iy)*nx + ix;
					border[index] = false;
					for(int dk = -1; dk <= 1; ++dk)
						for(int dj = -1; dj <= 1; ++dj)
							for(int di = -1; di <= 1; ++di)
							{
								int ci = ix + di;
								int cj = iy + dj;
								int ck = iz + dk;

								if(ci < 0) ci = 0; else if(ci > (nx-1)) ci = nx-1;
								if(cj < 0) cj = 0; else if(cj > (ny-1)) cj = ny-1;
								if(ck < 0) ck = 0; else if(ck > (nz-1)) ck = nz-1;

								if( ci < grids[i].sx || ci >= grids[i].ex ||
									cj < grids[i].sy || cj >= grids[i].ey ||
									ck < grids[i].sz || ck >= grids[i].ez )
									border[index] = true;
							}
				}

	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

	mutex = new pthread_mutex_t *[numCells];
	for(int i = 0; i < numCells; ++i)
	{
		int n = (border[i] ? 16 : 1);
		mutex[i] = new pthread_mutex_t[n];
		for(int j = 0; j < n; ++j)
		pthread_mutex_init(&mutex[i][j], NULL);
	}

	cells = new Cell[numCells];
	cells2 = new Cell[numCells];
	cnumPars = new int[numCells];
	cnumPars2 = new int[numCells];
	assert(cells && cells2 && cnumPars && cnumPars2);

	memset(cnumPars2, 0, numCells*sizeof(int));

	float px, py, pz, hvx, hvy, hvz, vx, vy, vz;
	for(int i = 0; i < origNumParticles; ++i)
	{
	  fread((void *)&px, sizeof(float), 1, fp);	  
	  fread((void *)&py, sizeof(float), 1, fp);	  
	  fread((void *)&pz, sizeof(float), 1, fp);	  
	  fread((void *)&hvx, sizeof(float), 1, fp);	  
	  fread((void *)&hvy, sizeof(float), 1, fp);	  
	  fread((void *)&hvz, sizeof(float), 1, fp);	  
	  fread((void *)&vx, sizeof(float), 1, fp);	  
	  fread((void *)&vy, sizeof(float), 1, fp);	  
	  fread((void *)&vz, sizeof(float), 1, fp);	  

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
	  if(np < 16) {
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
	  } else {
		--numParticles;
	  }
	}
	std::cout << "Number of particles: " << numParticles << " (" << origNumParticles-numParticles << " skipped)" << std::endl;
}

////////////////////////////////////////////////////////////////////////////////

void SaveFile(char const *fileName)
{
        FILE *file;
	std::cout << "Saving file \"" << fileName << "\"..." << std::endl;

	file = fopen(fileName, "w");
	assert(file);

        if(!isLittleEndian()) {
          float restParticlesPerMeter_le;
          int   origNumParticles_le;

          restParticlesPerMeter_le = bswap_float(restParticlesPerMeter);
          origNumParticles_le      = bswap_int32(origNumParticles);
          fwrite(&restParticlesPerMeter_le, sizeof(float), 1, file);
          fwrite(&origNumParticles_le, sizeof(int), 1, file);
        } else {
          fwrite(&restParticlesPerMeter, sizeof(float), 1, file);
          fwrite(&origNumParticles, sizeof(int), 1, file);
        }

	int count = 0;
	for(int i = 0; i < numCells; ++i)
	{
		Cell const &cell = cells[i];
		int np = cnumPars[i];
		for(int j = 0; j < np; ++j) {
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

                      fwrite(&px, sizeof(float), 1, file);
                      fwrite(&py, sizeof(float), 1, file);
                      fwrite(&pz, sizeof(float), 1, file);
                      fwrite(&hvx, sizeof(float), 1, file);
                      fwrite(&hvy, sizeof(float), 1, file);
                      fwrite(&hvz, sizeof(float), 1, file);
                      fwrite(&vx, sizeof(float), 1, file);
                      fwrite(&vy, sizeof(float), 1, file);
                      fwrite(&vz, sizeof(float), 1, file);
                    } else {
		      fwrite(&cell.p[j].x, sizeof(float), 1, file);
		      fwrite(&cell.p[j].y, sizeof(float), 1, file);
		      fwrite(&cell.p[j].z, sizeof(float), 1, file);
		      fwrite(&cell.hv[j].x, sizeof(float), 1, file);
		      fwrite(&cell.hv[j].y, sizeof(float), 1, file);
		      fwrite(&cell.hv[j].z, sizeof(float), 1, file);
		      fwrite(&cell.v[j].x, sizeof(float), 1, file);
		      fwrite(&cell.v[j].y, sizeof(float), 1, file);
		      fwrite(&cell.v[j].z, sizeof(float), 1, file);
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

	for(int i = 0; i < numSkipped; ++i) {
	  fwrite(&zero, sizeof(float), 1, file);
	  fwrite(&zero, sizeof(float), 1, file);
	  fwrite(&zero, sizeof(float), 1, file);
	  fwrite(&zero, sizeof(float), 1, file);
	  fwrite(&zero, sizeof(float), 1, file);
	  fwrite(&zero, sizeof(float), 1, file);
	  fwrite(&zero, sizeof(float), 1, file);
	  fwrite(&zero, sizeof(float), 1, file);
	  fwrite(&zero, sizeof(float), 1, file);
	}
}

////////////////////////////////////////////////////////////////////////////////

void CleanUpSim()
{
	pthread_attr_destroy(&attr);

	for(int i = 0; i < numCells; ++i)
	{
		int n = (border[i] ? 16 : 1);
		for(int j = 0; j < n; ++j)
			pthread_mutex_destroy(&mutex[i][j]);
		delete[] mutex[i];
	}
	delete[] mutex;

	delete[] border;

	delete[] cells;
	delete[] cells2;
	delete[] cnumPars;
	delete[] cnumPars2;
	delete[] thread;
	delete[] grids;
}

////////////////////////////////////////////////////////////////////////////////

#ifdef TBB_VERSION
class WorkerBase {
  char *myname;
public:
  void setName(char *name)
  {
    int len;
    if(myname)
      return;

    len = strlen(name)+1;
    myname = new char[len];
    strcpy(myname, name);
  }

  char *getName(){return myname;}
};

template <class T>
class WorkerLauncher: public tbb::task {
  bool is_continuation;
public:
  int i_int;
  WorkerLauncher(int i):i_int(i),is_continuation(false){}
  task *execute() {
    long long grid_i = (long long)i_int;
    tbb::task_list list;
    int sz;
    int ez;

    if(!is_continuation) {
      recycle_as_continuation();
      set_ref_count(NUM_TASKS);
      sz = grids[grid_i].sz;
      ez = grids[grid_i].ez;

      for(int t=1;t<NUM_TASKS;t++)
	list.push_back( *new( tbb::task::allocate_child() ) T(t, i_int, sz, ez)  );
      T &me = *new (tbb::task::allocate_child()) T(0,i_int,sz,ez);
      spawn(list);
      is_continuation = true;
      return &me;
    
    } else {
      return NULL;
    }
  }
};

template<class T>
class GridLauncher: public tbb::task {
  bool is_continuation;
  int num_grids;
public:
  GridLauncher(int n_):num_grids(n_), is_continuation(false)
  {
    //    fprintf(stderr,"Now launchig %d grids of \"%s\"\n",n_, T.getName());
  }

  task *execute() {
    tbb::task_list list;

    if(!is_continuation) {
      recycle_as_continuation();
      set_ref_count(num_grids);

      for(int i=1;i<num_grids;i++)
	list.push_back( *new( tbb::task::allocate_child() ) WorkerLauncher<T>(i) );
      WorkerLauncher<T> &me = *new (tbb::task::allocate_child()) WorkerLauncher<T>(0);
      spawn(list);
      is_continuation = true;
      return &me;
    
    } else {
      return NULL;
    }
  }
};


class ClearParticlesMTWorker: public tbb::task {
  char *name;
  int i_int;
  int pid;
  int start;
  int end;
public:
  ClearParticlesMTWorker(int id, int i_, int s, int e):
    pid(id), i_int(i_), start(s), end(e)
  {name="ClearParticles";}

  task *execute() {
    long long i = (long long)i_int;
    int block = (end - start)/NUM_TASKS;
    int sz = start + block*pid;
    int ez = sz + block;

    if(pid==(NUM_TASKS-1))
       ez = end;
       
    for(int iz = sz; iz < ez; ++iz)
      for(int iy = grids[i].sy; iy < grids[i].ey; ++iy)
	for(int ix = grids[i].sx; ix < grids[i].ex; ++ix)
	  {
	    int index = (iz*ny + iy)*nx + ix;
	    cnumPars[index] = 0;
	  }


    return NULL;
  }
  
  char *getName(){return name;}

};
#endif //TBB_VERSION


void *ClearParticlesMT(void *tid)
{
	long long i = (long long)(tid);

	for(int iz = grids[i].sz; iz < grids[i].ez; ++iz)
		for(int iy = grids[i].sy; iy < grids[i].ey; ++iy)
			for(int ix = grids[i].sx; ix < grids[i].ex; ++ix)
			{
			  int index = (iz*ny + iy)*nx + ix;
			  cnumPars[index] = 0;
			}


	return 0;
}

////////////////////////////////////////////////////////////////////////////////

#ifdef TBB_VERSION

class RebuildGridMTWorker: public tbb::task {
  char *name;
  int i_int;
  int pid;
  int start;
  int end;
public:
  RebuildGridMTWorker(int id, int i_, int s, int e):
    pid(id), i_int(i_), start(s), end(e){name="RebuildGrid";}

  task *execute() {
    long long i = (long long)i_int;
    int block = (end - start)/NUM_TASKS;
    int sz = start + block*pid;
    int ez = sz + block;

    if(pid==(NUM_TASKS-1))
       ez = end;
       
    for(int iz = sz; iz < ez; ++iz)
      for(int iy = grids[i].sy; iy < grids[i].ey; ++iy)
	for(int ix = grids[i].sx; ix < grids[i].ex; ++ix)
	  {
	    int index = (iz*ny + iy)*nx + ix;
	    Cell const &cell2 = cells2[index];
	    int np2 = cnumPars2[index];
	    for(int j = 0; j < np2; ++j)
	      {
		int ci = (int)((cell2.p[j].x - domainMin.x) / delta.x);
		int cj = (int)((cell2.p[j].y - domainMin.y) / delta.y);
		int ck = (int)((cell2.p[j].z - domainMin.z) / delta.z);
		
		if(ci < 0) ci = 0; else if(ci > (nx-1)) ci = nx-1;
		if(cj < 0) cj = 0; else if(cj > (ny-1)) cj = ny-1;
		if(ck < 0) ck = 0; else if(ck > (nz-1)) ck = nz-1;
		
		int index2 = (ck*ny + cj)*nx + ci;

		// this assumes that particles cannot travel more than one grid cell per time step
		int np;
		if(border[index2])
		  {
		    pthread_mutex_lock(&mutex[index2][0]);
		    np = cnumPars[index2]++;
		    pthread_mutex_unlock(&mutex[index2][0]);
		  }
		else
		  np = cnumPars[index2]++;

		Cell &cell = cells[index2];
		cell.p[np].x = cell2.p[j].x;
		cell.p[np].y = cell2.p[j].y;
		cell.p[np].z = cell2.p[j].z;
		cell.hv[np].x = cell2.hv[j].x;
		cell.hv[np].y = cell2.hv[j].y;
		cell.hv[np].z = cell2.hv[j].z;
		cell.v[np].x = cell2.v[j].x;
		cell.v[np].y = cell2.v[j].y;
		cell.v[np].z = cell2.v[j].z;
	      }
	  }
    return NULL;
  }
  
  char *getName(){return name;}

};
#endif // TBB_VERSION

void *RebuildGridMT(void *tid)
{
	long long i = (long long)(tid);

	for(int iz = grids[i].sz; iz < grids[i].ez; ++iz)
		for(int iy = grids[i].sy; iy < grids[i].ey; ++iy)
			for(int ix = grids[i].sx; ix < grids[i].ex; ++ix)
			{
			  int index = (iz*ny + iy)*nx + ix;
				Cell const &cell2 = cells2[index];
				int np2 = cnumPars2[index];
				for(int j = 0; j < np2; ++j)
				{
					int ci = (int)((cell2.p[j].x - domainMin.x) / delta.x);
					int cj = (int)((cell2.p[j].y - domainMin.y) / delta.y);
					int ck = (int)((cell2.p[j].z - domainMin.z) / delta.z);

					if(ci < 0) ci = 0; else if(ci > (nx-1)) ci = nx-1;
					if(cj < 0) cj = 0; else if(cj > (ny-1)) cj = ny-1;
					if(ck < 0) ck = 0; else if(ck > (nz-1)) ck = nz-1;

					int index2 = (ck*ny + cj)*nx + ci;
					// this assumes that particles cannot travel more than one grid cell per time step
					int np;
					if(border[index2])
					{
						pthread_mutex_lock(&mutex[index2][0]);
						np = cnumPars[index2]++;
						pthread_mutex_unlock(&mutex[index2][0]);
					}
					else
						np = cnumPars[index2]++;

					Cell &cell = cells[index2];
					cell.p[np].x = cell2.p[j].x;
					cell.p[np].y = cell2.p[j].y;
					cell.p[np].z = cell2.p[j].z;
					cell.hv[np].x = cell2.hv[j].x;
					cell.hv[np].y = cell2.hv[j].y;
					cell.hv[np].z = cell2.hv[j].z;
					cell.v[np].x = cell2.v[j].x;
					cell.v[np].y = cell2.v[j].y;
					cell.v[np].z = cell2.v[j].z;
				}
			}




	return 0;
}

////////////////////////////////////////////////////////////////////////////////

int InitNeighCellList(int ci, int cj, int ck, int *neighCells)
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

#ifdef TBB_VERSION

class InitDensitiesAndForcesMTWorker: public tbb::task {
  char *name;
  int i_int;
  int pid;
  int start;
  int end;
public:
  InitDensitiesAndForcesMTWorker(int id, int i_, int s, int e):
    pid(id), i_int(i_), start(s), end(e){name="InitDensitiesAndForce";}
  char *getName(){return name;}

  task *execute() {
    long long i = (long long)i_int;
    int block = (end - start)/NUM_TASKS;
    int sz = start + block*pid;
    int ez = sz + block;

    if(pid==(NUM_TASKS-1))
       ez = end;
       
    for(int iz = sz; iz < ez; ++iz)
      {
	
	for(int iy = grids[i].sy; iy < grids[i].ey; ++iy)
	  for(int ix = grids[i].sx; ix < grids[i].ex; ++ix)
	    {
	      int index = (iz*ny + iy)*nx + ix;
	      Cell &cell = cells[index];
	      int np = cnumPars[index];
	      for(int j = 0; j < np; ++j)
		{
		  cell.density[j] = 0.f;
		  cell.a[j] = externalAcceleration;
		}
	    }
      } // for
    return NULL;
  }
  
};
#endif //TBB_VERSION


void *InitDensitiesAndForcesMT(void *tid)
{
	long long i = (long long)(tid);
	for(int iz = grids[i].sz; iz < grids[i].ez; ++iz)
		for(int iy = grids[i].sy; iy < grids[i].ey; ++iy)
			for(int ix = grids[i].sx; ix < grids[i].ex; ++ix)
			{
		                int index = (iz*ny + iy)*nx + ix;
				Cell &cell = cells[index];
				int np = cnumPars[index];
				for(int j = 0; j < np; ++j)
				{
					cell.density[j] = 0.f;
					cell.a[j] = externalAcceleration;
				}
			}

	return 0;
}

////////////////////////////////////////////////////////////////////////////////

#ifdef TBB_VERSION

class ComputeDensitiesMTWorker: public tbb::task {
  char *name;
  int i_int;
  int pid;
  int start;
  int end;
public:
  ComputeDensitiesMTWorker(int id, int i_, int s, int e):
    pid(id), i_int(i_), start(s), end(e){name="ComputeDensities";}
  char *getName(){return name;}

  task *execute() {
    int neighCells[27];
    long long i = (long long)i_int;
    int block = (end - start)/NUM_TASKS;
    int sz = start + block*pid;
    int ez = sz + block;

    if(pid==(NUM_TASKS-1))
       ez = end;

    for(int iz = sz; iz < ez; ++iz)
      {
	
	for(int iy = grids[i].sy; iy < grids[i].ey; ++iy)
	  for(int ix = grids[i].sx; ix < grids[i].ex; ++ix)
	    {
	      int index = (iz*ny + iy)*nx + ix;
	      int np = cnumPars[index];
	      if(np == 0)
		continue;
	      
	      int numNeighCells = InitNeighCellList(ix, iy, iz, neighCells);
	      
	      Cell &cell = cells[index];
	      
	      for(int j = 0; j < np; ++j)
		for(int inc = 0; inc < numNeighCells; ++inc)
		  {
		    int indexNeigh = neighCells[inc];
		    Cell &neigh = cells[indexNeigh];
		    int numNeighPars = cnumPars[indexNeigh];
		    for(int iparNeigh = 0; iparNeigh < numNeighPars; ++iparNeigh)
		      if(&neigh.p[iparNeigh] < &cell.p[j])
			{
			  float distSq = (cell.p[j] - neigh.p[iparNeigh]).GetLengthSq();
			  if(distSq < hSq)
			    {
			      float t = hSq - distSq;
			      float tc = t*t*t;
			      
			      if(border[index])
				{
				  pthread_mutex_lock(&mutex[index][j]);
				  cell.density[j] += tc;
				  pthread_mutex_unlock(&mutex[index][j]);
				}
			      else
				cell.density[j] += tc;
			      
			      if(border[indexNeigh])
				{
				  pthread_mutex_lock(&mutex[indexNeigh][iparNeigh]);
				  neigh.density[iparNeigh] += tc;
				  pthread_mutex_unlock(&mutex[indexNeigh][iparNeigh]);
				}
			      else
				neigh.density[iparNeigh] += tc;
			    }
			}
		  }
	    }
	
      } // for z

    return NULL;
  }
  
};

#endif //TBB_VERSION


void *ComputeDensitiesMT(void *tid)
{
	long long i = (long long)(tid);

	int neighCells[27];

	for(int iz = grids[i].sz; iz < grids[i].ez; ++iz)
		for(int iy = grids[i].sy; iy < grids[i].ey; ++iy)
			for(int ix = grids[i].sx; ix < grids[i].ex; ++ix)
			{
                		int index = (iz*ny + iy)*nx + ix;
				int np = cnumPars[index];
				if(np == 0)
				  continue;

				int numNeighCells = InitNeighCellList(ix, iy, iz, neighCells);

				Cell &cell = cells[index];

				for(int j = 0; j < np; ++j)
					for(int inc = 0; inc < numNeighCells; ++inc)
					{
						int indexNeigh = neighCells[inc];
						Cell &neigh = cells[indexNeigh];
						int numNeighPars = cnumPars[indexNeigh];
						for(int iparNeigh = 0; iparNeigh < numNeighPars; ++iparNeigh)
							if(&neigh.p[iparNeigh] < &cell.p[j])
							{
								float distSq = (cell.p[j] - neigh.p[iparNeigh]).GetLengthSq();
								if(distSq < hSq)
								{
									float t = hSq - distSq;
									float tc = t*t*t;

									if(border[index])
									{
										pthread_mutex_lock(&mutex[index][j]);
										cell.density[j] += tc;
										pthread_mutex_unlock(&mutex[index][j]);
									}
									else
									  cell.density[j] += tc;

									if(border[indexNeigh])
									{
									  pthread_mutex_lock(&mutex[indexNeigh][iparNeigh]);
									  neigh.density[iparNeigh] += tc;
									  pthread_mutex_unlock(&mutex[indexNeigh][iparNeigh]);
									}
									else
									  neigh.density[iparNeigh] += tc;
								}
							}
					}
			}

	return 0;
}

////////////////////////////////////////////////////////////////////////////////

#ifdef TBB_VERSION

class ComputeDensities2MTWorker: public tbb::task {
  char *name;
  int i_int;
  int pid;
  int start;
  int end;
public:
  ComputeDensities2MTWorker(int id, int i_, int s, int e):
    pid(id), i_int(i_), start(s), end(e){name="ComputeDensities2";}
  char *getName(){return name;}
  
  task *execute() {
    const float tc = hSq*hSq*hSq;
    long long i = (long long)i_int;
    int block = (end - start)/NUM_TASKS;
    int sz = start + block*pid;
    int ez = sz + block;

    if(pid==(NUM_TASKS-1))
       ez = end;

    for(int iz = sz; iz < ez; ++iz)
      {
	for(int iy = grids[i].sy; iy < grids[i].ey; ++iy)
	  for(int ix = grids[i].sx; ix < grids[i].ex; ++ix)
	    {
	      int index = (iz*ny + iy)*nx + ix;
	      Cell &cell = cells[index];
	      int np = cnumPars[index];
	      for(int j = 0; j < np; ++j)
		{
		  cell.density[j] += tc;
		  cell.density[j] *= densityCoeff;
		}
	    }	
	
      } // for
    return NULL;
  }
  
};
#endif

void *ComputeDensities2MT(void *tid)
{
	long long i = (long long)(tid);

	const float tc = hSq*hSq*hSq;
	for(int iz = grids[i].sz; iz < grids[i].ez; ++iz)
		for(int iy = grids[i].sy; iy < grids[i].ey; ++iy)
			for(int ix = grids[i].sx; ix < grids[i].ex; ++ix)
			{
                int index = (iz*ny + iy)*nx + ix;
				Cell &cell = cells[index];
				int np = cnumPars[index];
				for(int j = 0; j < np; ++j)
				{
					cell.density[j] += tc;
					cell.density[j] *= densityCoeff;
				}
			}


	return 0;
}

////////////////////////////////////////////////////////////////////////////////
#ifdef TBB_VERSION

class ComputeForcesMTWorker: public tbb::task {
  char *name;
  int i_int;
  int pid;
  int start;
  int end;
public:
  ComputeForcesMTWorker(int id, int i_, int s, int e):
    pid(id), i_int(i_), start(s), end(e){name="ComputeForce";}
  char *getName(){return name;}

  task *execute() {
    int neighCells[27];
    long long i = (long long)i_int;
    int block = (end - start)/NUM_TASKS;
    int sz = start + block*pid;
    int ez = sz + block;

    if(pid==(NUM_TASKS-1))
       ez = end;

    for(int iz = sz; iz < ez; ++iz)
      {

	
	for(int iy = grids[i].sy; iy < grids[i].ey; ++iy)
	  for(int ix = grids[i].sx; ix < grids[i].ex; ++ix)
	    {
	      int index = (iz*ny + iy)*nx + ix;
	      int np = cnumPars[index];
	      if(np == 0)
		continue;
	      
	      int numNeighCells = InitNeighCellList(ix, iy, iz, neighCells);
	      
	      Cell &cell = cells[index];
	      
	      for(int j = 0; j < np; ++j)
		for(int inc = 0; inc < numNeighCells; ++inc)
		  {
		    int indexNeigh = neighCells[inc];
		    Cell &neigh = cells[indexNeigh];
		    int numNeighPars = cnumPars[indexNeigh];
		    for(int iparNeigh = 0; iparNeigh < numNeighPars; ++iparNeigh)
		      if(&neigh.p[iparNeigh] < &cell.p[j])
			{
			  Vec3 disp = cell.p[j] - neigh.p[iparNeigh];
			  float distSq = disp.GetLengthSq();
			  if(distSq < hSq)
			    {
			      float dist = sqrtf(std::max(distSq, 1e-12f));
			      float hmr = h - dist;
			      
			      Vec3 acc = disp * pressureCoeff * (hmr*hmr/dist) * (cell.density[j]+neigh.density[iparNeigh] - doubleRestDensity);
			      acc += (neigh.v[iparNeigh] - cell.v[j]) * viscosityCoeff * hmr;
			      acc /= cell.density[j] * neigh.density[iparNeigh];
			      
			      if(border[index])
				{
				  pthread_mutex_lock(&mutex[index][j]);
				  cell.a[j] += acc;
				  pthread_mutex_unlock(&mutex[index][j]);
				}
			      else
				cell.a[j] += acc;
			      
			      if(border[indexNeigh])
				{
				  pthread_mutex_lock(&mutex[indexNeigh][iparNeigh]);
				  neigh.a[iparNeigh] -= acc;
				  pthread_mutex_unlock(&mutex[indexNeigh][iparNeigh]);
				}
			      else
				neigh.a[iparNeigh] -= acc;
			    }
			}
		  }
	    }
	
	
      } // for
    return NULL;
  }
  
};
#endif //TBB_VERSION


void *ComputeForcesMT(void *tid)
{
	long long i = (long long)(tid);

	int neighCells[27];

	for(int iz = grids[i].sz; iz < grids[i].ez; ++iz)
		for(int iy = grids[i].sy; iy < grids[i].ey; ++iy)
			for(int ix = grids[i].sx; ix < grids[i].ex; ++ix)
			{
                int index = (iz*ny + iy)*nx + ix;
				int np = cnumPars[index];
				if(np == 0)
					continue;

				int numNeighCells = InitNeighCellList(ix, iy, iz, neighCells);

				Cell &cell = cells[index];

				for(int j = 0; j < np; ++j)
					for(int inc = 0; inc < numNeighCells; ++inc)
					{
						int indexNeigh = neighCells[inc];
						Cell &neigh = cells[indexNeigh];
						int numNeighPars = cnumPars[indexNeigh];
						for(int iparNeigh = 0; iparNeigh < numNeighPars; ++iparNeigh)
							if(&neigh.p[iparNeigh] < &cell.p[j])
							{
								Vec3 disp = cell.p[j] - neigh.p[iparNeigh];
								float distSq = disp.GetLengthSq();
								if(distSq < hSq)
								{
									float dist = sqrtf(std::max(distSq, 1e-12f));
									float hmr = h - dist;

									Vec3 acc = disp * pressureCoeff * (hmr*hmr/dist) * (cell.density[j]+neigh.density[iparNeigh] - doubleRestDensity);
									acc += (neigh.v[iparNeigh] - cell.v[j]) * viscosityCoeff * hmr;
									acc /= cell.density[j] * neigh.density[iparNeigh];

									if(border[index])
									{
										pthread_mutex_lock(&mutex[index][j]);
										cell.a[j] += acc;
										pthread_mutex_unlock(&mutex[index][j]);
									}
									else
										cell.a[j] += acc;

									if(border[indexNeigh])
									{
										pthread_mutex_lock(&mutex[indexNeigh][iparNeigh]);
										neigh.a[iparNeigh] -= acc;
										pthread_mutex_unlock(&mutex[indexNeigh][iparNeigh]);
									}
									else
										neigh.a[iparNeigh] -= acc;
								}
							}
					}
			}

	return 0;
}

////////////////////////////////////////////////////////////////////////////////


#ifdef TBB_VERSION
class ProcessCollisionsMTWorker: public tbb::task {
  char *name;
  int i_int;
  int pid;
  int start;
  int end;
public:
  ProcessCollisionsMTWorker(int id, int i_, int s, int e):
    pid(id), i_int(i_), start(s), end(e){name="ProcessCollisions";}
  char *getName(){return name;}

  task *execute() {
    const float parSize = 0.0002f;
    const float epsilon = 1e-10f;
    const float stiffness = 30000.f;
    const float damping = 128.f;
    
    long long i = (long long)i_int;
    int block = (end - start)/NUM_TASKS;
    int sz = start + block*pid;
    int ez = sz + block;

    if(pid==(NUM_TASKS-1))
       ez = end;
       
    for(int iz = sz; iz < ez; ++iz)
      {
	
	for(int iy = grids[i].sy; iy < grids[i].ey; ++iy)
	  for(int ix = grids[i].sx; ix < grids[i].ex; ++ix)
	    {
	      int index = (iz*ny + iy)*nx + ix;
	      Cell &cell = cells[index];
	      int np = cnumPars[index];
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
      } // for
    return NULL;
  }
};
#endif //TBB_VERSION

void *ProcessCollisionsMT(void *tid)
{

  long long i = (long long)(tid);
  const float parSize = 0.0002f;
  const float epsilon = 1e-10f;
  const float stiffness = 30000.f;
  const float damping = 128.f;
  
	for(int iz = grids[i].sz; iz < grids[i].ez; ++iz)
		for(int iy = grids[i].sy; iy < grids[i].ey; ++iy)
			for(int ix = grids[i].sx; ix < grids[i].ex; ++ix)
			{
                int index = (iz*ny + iy)*nx + ix;
				Cell &cell = cells[index];
				int np = cnumPars[index];
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

	return 0;
}

////////////////////////////////////////////////////////////////////////////////

#ifdef TBB_VERSION
class AdvanceParticlesMTWorker: public tbb::task {
  char *name;
  int i_int;
  int pid;
  int start;
  int end;
public:
  AdvanceParticlesMTWorker(int id, int i_, int s, int e):
    pid(id), i_int(i_), start(s), end(e){name="AdvanceParticle";}
  char *getName(){return name;}

  task *execute() {
    long long i = (long long)i_int;
    int block = (end - start)/NUM_TASKS;
    int sz = start + block*pid;
    int ez = sz + block;

    if(pid==(NUM_TASKS-1))
       ez = end;

    for(int iz = sz; iz < ez; ++iz)
      {

	for(int iy = grids[i].sy; iy < grids[i].ey; ++iy)
	  for(int ix = grids[i].sx; ix < grids[i].ex; ++ix)
	    {
	      int index = (iz*ny + iy)*nx + ix;
	      Cell &cell = cells[index];
	      int np = cnumPars[index];
	      for(int j = 0; j < np; ++j)
		{
		  Vec3 v_half = cell.hv[j] + cell.a[j]*timeStep;
		  cell.p[j] += v_half * timeStep;
		  cell.v[j] = cell.hv[j] + v_half;
		  cell.v[j] *= 0.5f;
		  cell.hv[j] = v_half;
		}
	    }
	
      } // for
    return NULL;
  }
  
};
#endif //TBB_VERSION

void *AdvanceParticlesMT(void *tid)
{
	long long i = (long long)(tid);

	for(int iz = grids[i].sz; iz < grids[i].ez; ++iz)
		for(int iy = grids[i].sy; iy < grids[i].ey; ++iy)
			for(int ix = grids[i].sx; ix < grids[i].ex; ++ix)
			{
			        int index = (iz*ny + iy)*nx + ix;
				Cell &cell = cells[index];
				int np = cnumPars[index];
				for(int j = 0; j < np; ++j)
				{
					Vec3 v_half = cell.hv[j] + cell.a[j]*timeStep;
					cell.p[j] += v_half * timeStep;
					cell.v[j] = cell.hv[j] + v_half;
					cell.v[j] *= 0.5f;
					cell.hv[j] = v_half;
				}
			}

	return 0;
}

////////////////////////////////////////////////////////////////////////////////

#ifdef TBB_VERSION
void AdvanceFrame()
{
	void *status;

	GridLauncher<ClearParticlesMTWorker> &cp = *new (tbb::task::allocate_root()) GridLauncher<ClearParticlesMTWorker>(NUM_GRIDS);
	tbb::task::spawn_root_and_wait(cp);

	GridLauncher<RebuildGridMTWorker> &rg = *new (tbb::task::allocate_root()) GridLauncher<RebuildGridMTWorker>(NUM_TBB_GRIDS);
	tbb::task::spawn_root_and_wait(rg);

	GridLauncher<InitDensitiesAndForcesMTWorker> &id = *new (tbb::task::allocate_root()) GridLauncher<InitDensitiesAndForcesMTWorker>(NUM_TBB_GRIDS);
	tbb::task::spawn_root_and_wait(id);

	GridLauncher<ComputeDensitiesMTWorker> &cd = *new (tbb::task::allocate_root()) GridLauncher<ComputeDensitiesMTWorker>(NUM_TBB_GRIDS);
	tbb::task::spawn_root_and_wait(cd);

	GridLauncher<ComputeDensities2MTWorker> &cd2 = *new (tbb::task::allocate_root()) GridLauncher<ComputeDensities2MTWorker>(NUM_TBB_GRIDS);
	tbb::task::spawn_root_and_wait(cd2);

	GridLauncher<ComputeForcesMTWorker> &cf = *new (tbb::task::allocate_root()) GridLauncher<ComputeForcesMTWorker>(NUM_TBB_GRIDS);
	tbb::task::spawn_root_and_wait(cf);

	GridLauncher<ProcessCollisionsMTWorker> &pc = *new (tbb::task::allocate_root()) GridLauncher<ProcessCollisionsMTWorker>(NUM_TBB_GRIDS);
	tbb::task::spawn_root_and_wait(pc);

	GridLauncher<AdvanceParticlesMTWorker> &ap = *new (tbb::task::allocate_root()) GridLauncher<AdvanceParticlesMTWorker>(NUM_GRIDS);
	tbb::task::spawn_root_and_wait(ap);

}
#else
void AdvanceFrame()
{
	void *status;

	for(int i = 0; i < NUM_GRIDS; ++i)
		pthread_create(&thread[i], &attr, ClearParticlesMT, (void *)i);
	for(int i = 0; i < NUM_GRIDS; ++i)
		pthread_join(thread[i], &status);

	for(int i = 0; i < NUM_GRIDS; ++i)
		pthread_create(&thread[i], &attr, RebuildGridMT, (void *)i);
	for(int i = 0; i < NUM_GRIDS; ++i)
		pthread_join(thread[i], &status);

	for(int i = 0; i < NUM_GRIDS; ++i)
		pthread_create(&thread[i], &attr, InitDensitiesAndForcesMT, (void *)i);
	for(int i = 0; i < NUM_GRIDS; ++i)
		pthread_join(thread[i], &status);

	for(int i = 0; i < NUM_GRIDS; ++i)
		pthread_create(&thread[i], &attr, ComputeDensitiesMT, (void *)i);
	for(int i = 0; i < NUM_GRIDS; ++i)
		pthread_join(thread[i], &status);

	for(int i = 0; i < NUM_GRIDS; ++i)
		pthread_create(&thread[i], &attr, ComputeDensities2MT, (void *)i);
	for(int i = 0; i < NUM_GRIDS; ++i)
		pthread_join(thread[i], &status);

	for(int i = 0; i < NUM_GRIDS; ++i)
		pthread_create(&thread[i], &attr, ComputeForcesMT, (void *)i);
	for(int i = 0; i < NUM_GRIDS; ++i)
		pthread_join(thread[i], &status);

	for(int i = 0; i < NUM_GRIDS; ++i)
		pthread_create(&thread[i], &attr, ProcessCollisionsMT, (void *)i);
	for(int i = 0; i < NUM_GRIDS; ++i)
		pthread_join(thread[i], &status);

	for(int i = 0; i < NUM_GRIDS; ++i)
		pthread_create(&thread[i], &attr, AdvanceParticlesMT, (void *)i);
	for(int i = 0; i < NUM_GRIDS; ++i)
		pthread_join(thread[i], &status);
}

#endif //TBB_VERSION


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
	if(threadnum < 1) {
		std::cerr << "<threadnum> must at least be 1" << std::endl;
		return -1;
	}
	if(framenum < 1) {
		std::cerr << "<framenum> must at least be 1" << std::endl;
		return -1;
	}

#ifdef TBB_VERSION
	NUM_TBB_GRIDS=threadnum;
	tbb::task_scheduler_init init(threadnum);
#endif

	InitSim(argv[3], threadnum);

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
