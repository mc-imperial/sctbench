$!
$! Make ImageMagick image utilities for VMS.
$!
$ define/nolog MAGICK [-.magick]
$ define/nolog WAND [-.wand]
$ copy config.h_vms magick-config.h
$ copy xwdfile.h_vms xwdfile.h
$
$if (f$trnlnm("X11") .eqs. "") then define/nolog X11 decw$include:
$compile_options="/nodebug/optimize"
$if (f$search("sys$system:decc$compiler.exe") .nes. "") 
$then     ! VAX with DEC C
$  compile_options="/decc/nodebug/optimize/warning=(disable=rightshiftovr)"
$else     ! VAX with VAX C
$define/nolog lnk$library sys$library:vaxcrtl
$define/nolog sys sys$share
$endif
$if (f$getsyi("HW_MODEL") .gt. 1023)
$then     ! Alpha with DEC C
$  define/nolog sys decc$library_include
$  compile_options="/nodebug/optimize/prefix=all/warning=(disable=rightshiftovr)/name=(as_is,short)/float=ieee
$endif
$
$write sys$output "Making Magick..."
$call Make animate.c
$call Make annotate.c
$call Make artifact.c
$call Make blob.c
$call Make cache.c
$call Make cache-view.c
$call Make client.c
$call Make coder.c
$call Make color.c
$call Make colorspace.c
$call Make compare.c
$call Make composite.c
$call Make compress.c
$call Make configure.c
$call Make constitute.c
$call Make decorate.c
$call Make delegate.c
$call Make deprecate.c
$call Make display.c
$call Make distort.c
$call Make draw.c
$call Make effect.c
$call Make enhance.c
$call Make exception.c
$call Make fx.c
$call Make gem.c
$call Make geometry.c
$call Make hashmap.c
$call Make identify.c
$call Make image.c
$call Make layer.c
$call Make list.c
$call Make locale.c
$call Make log.c
$call Make magic.c
$call Make magick.c
$call Make matrix.c
$call Make memory.c
$call Make mime.c
$call Make module.c
$call Make monitor.c
$call Make montage.c
$call Make option.c
$call Make paint.c
$call Make pixel.c
$call Make prepress.c
$call Make property.c
$call Make PreRvIcccm.c
$call Make profile.c
$call Make quantize.c
$call Make quantum.c
$call Make random.c
$call Make registry.c
$call Make resample.c
$call Make resize.c
$call Make resource.c
$call Make segment.c
$call Make semaphore.c
$call Make shear.c
$call Make signature.c
$call Make splay-tree.c
$call Make static.c
$call Make statistic.c
$call Make stream.c
$call Make string.c
$call Make timer.c
$call Make token.c
$call Make transform.c
$call Make threshold.c
$call Make type.c
$call Make utility.c
$call Make version.c
$call Make vms.c
$call Make widget.c
$call Make xml-tree.c
$call Make xwindow.c
$ set default [-.filters]
$ call Make analyze.c
$ set default [-.wand]
$ call Make drawing-wand.c
$ call Make pixel-wand.c
$ call Make conjure.c
$ call Make convert.c
$ call Make import.c
$ call Make mogrify.c
$ call make animate.c
$ call make compare.c
$ call make composite.c
$ call make display.c
$ call make identify.c
$ call make montage.c
$ set default [-.magick]
$ deass magick
$ deass wand
$library/create libMagick.olb -
  animate, annotate, artifact, blob, cache, cache-view, client, coder, -
  color, colorspace, compare, composite, compress, configure, -
  constitute, decorate, delegate, deprecate, display, distort, draw, -
  effect, enhance, exception, fx, gem, geometry, hashmap, identify, image, -
  layer, list, locale, log, magic, magick, matrix, memory, mime, module, -
  monitor, montage, option, paint, pixel, PreRvIcccm, profile, quantize, -
  quantum,random, registry, resample, resize, resource, segment, semaphore, -
  shear, signature, splay-tree, static, stream, string, timer, token, -
  transform, threshold, type, utility, version, vms, widget, xwindow, -
	statistics, prepress, property, xml-tree, -
	[-.filters]analyze,[-.wand]drawing-wand, pixel-wand, conjure, convert,-
	import, mogrify, animate, compare, composite, display, identify,-
	montage
$exit
$
$Make: subroutine
$!
$! Primitive MMS hack for DCL.
$!
$if (p1 .eqs. "") then exit
$source_file=f$search(f$parse(p1,".c"))
$if (source_file .nes. "")
$then
$  object_file=f$parse(source_file,,,"name")+".obj"
$  object_file=f$search( object_file )
$  if (object_file .nes. "")
$  then
$    object_time=f$file_attribute(object_file,"cdt")
$    source_time=f$file_attribute(source_file,"cdt")
$    if (f$cvtime(object_time) .lts. f$cvtime(source_time)) then -
$      object_file=""
$  endif
$  if (object_file .eqs. "")
$  then
$    write sys$output "Compiling ",p1
$    cc'compile_options'/include_directory=([-],[-.jpeg],[-.png], -
       [-.tiff],[-.ttf],[-.zlib]) 'source_file'  
$  endif
$endif
$exit
$endsubroutine
