// This may look like C code, but it is really -*- C++ -*-
//
// Copyright Bob Friesenhahn, 1999, 2000, 2003
//
// Test STL averageImages function
//

#include <Magick++.h>
#include <string>
#include <iostream>
#include <list>
#include <vector>

using namespace std;

using namespace Magick;

int main( int /*argc*/, char ** argv)
{

  // Initialize ImageMagick install location for Windows
  InitializeMagick(*argv);

  int failures=0;

  try {

    string srcdir("");
    if(getenv("SRCDIR") != 0)
      srcdir = getenv("SRCDIR");

    //
    // Test averageImages
    //
    
    list<Image> imageList;
    readImages( &imageList, srcdir + "test_image_anim.miff" );
    
    Image averaged;
    averageImages( &averaged, imageList.begin(), imageList.end() );
    // averaged.display();
    if ( averaged.signature() != "89abcbf9902b5f06f77e9cc2131f3ca209b563c05f534cf51b096805fdf57592" &&
         averaged.signature() != "22eae913dd66c712e2b2947a3856a66bd00d9622ed01ae85b0fa08f5a5941b0a" &&
         averaged.signature() != "514c6491fcc76308dac98aa7e3bca4b82036dd1c1a87b085f2f0e1a7a10e734d" &&
         averaged.signature() != "8a3cb3d44c4e5cde0e6dc6f06decf16be6e3cbf337abe8902cadbf381075b403" &&
         averaged.signature() != "f3bc318abc0b842c656b6545d1d7159eedb61f559a95fc5df671db7d0c0639de")
      {
	cout << "Line: " << __LINE__
	     << "  Averaging image failed, signature = "
	     << averaged.signature() << endl;
	averaged.display();
	++failures;
      }
  }

  catch( Exception &error_ )
    {
      cout << "Caught exception: " << error_.what() << endl;
      return 1;
    }
  catch( exception &error_ )
    {
      cout << "Caught exception: " << error_.what() << endl;
      return 1;
    }

  if ( failures )
    {
      cout << failures << " failures" << endl;
      return 1;
    }
  
  return 0;
}

