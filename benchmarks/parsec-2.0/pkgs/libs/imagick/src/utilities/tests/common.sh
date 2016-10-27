SRCDIR=`dirname $0`
SRCDIR=`cd $SRCDIR && pwd`
TOPSRCDIR=`cd $srcdir && pwd`
mkdir -p utilities/tests
cd utilities/tests || exit 1
MODEL_MIFF="${TOPSRCDIR}/Magick++/demo/model.miff"
SMILE_MIFF="${TOPSRCDIR}/Magick++/demo/smile.miff"
GENERIC_TTF="${TOPSRCDIR}/PerlMagick/demo/Generic.ttf"
