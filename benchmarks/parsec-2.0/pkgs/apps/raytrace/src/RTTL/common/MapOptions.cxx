#include "RTTL/common/MapOptions.hxx"

/// Instantiates MapOptions entry which is visible in any file which includes MapOptions.hxx.
/// \note The preferred way to parse command line parameters is to add them to options as
/// <b>options.parse(argc-1, argv+1);</b>
/// In this case, this variable must be defined (for example, by including this file in the project).
MapOptions options;
