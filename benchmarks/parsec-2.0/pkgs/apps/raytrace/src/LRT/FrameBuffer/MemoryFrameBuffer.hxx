#ifndef RTL__MEMORY_FRAMEBUFFER_HXX
#define RTL__MEMORY_FRAMEBUFFER_HXX

#include "../FrameBuffer.hxx"

namespace LRT BEGIN_NAMESPACE

// =======================================================
/*! simply using a continous region in memory as framebuffer,
    mostly useful for debugging and no display rendering
*/
struct MemoryFrameBuffer : public FrameBuffer
{
  int size;
  /*! try allocating one -- if that fails, return NULL */
  static MemoryFrameBuffer *create() {
    return new MemoryFrameBuffer;
  }

  MemoryFrameBuffer() {
    fb = NULL;
    size = 0;
  }
  virtual void resize(int newX, int newY)
  {
    FrameBuffer::resize(newX,newY);
    if (fb) aligned_free(fb);
    size = 4*res.x*res.y;
    fb = aligned_malloc<unsigned char>(size);
  }

  virtual void startNewFrame() {}
  virtual void doneWithFrame() {
    assert(fb);
    {
      ofstream myFile ("output.bin", ios::out | ios::binary);
      myFile.write (fb, size);
      myFile.flush();
    }
    std::cout << "Done with frame" << std::endl;
  }
  virtual void display() {}
};

END_NAMESPACE

#endif
