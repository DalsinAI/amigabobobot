/* BoboBot keeps its game in globals, but a Shell's 4 KB is too little for
 * SDL and the C library's stdio; libnix's startup sets this much up.
 * The AmiCradle Team. GPL 2, like BoboBot. */
unsigned long __stack = 128 * 1024;
