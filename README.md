# PylonSample_Stereo_Acquisition_PTP
Acquires images from two GigE cameras synchronized via IEEE1588 and stitches them side-by-side (does not do stereo processing, just acquisition)
On Windows, it uses Pylon's built-in libraries for recording images to .mp4 or .avi movies.

On Linux, it uses OpenCV's libraries to record .avi and Pylon's libraries to record to .mp4.
(note that for .mp4 recording, an additional package must be downloaded from www.baslerweb.com)