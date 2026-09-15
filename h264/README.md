# H264 Simple Sample Code

convert your video into frame.h264 file using something like:  

```bash
ffmpeg -i video.mp4 -an -c:v libx264 -profile:v main \
  -vf "scale=(iw*272/ih):272,crop=480:272" \
  -r 30 \
  -bsf:v "filter_units=remove_types=6|12|35|38-40" \
  -x264-params "bframes=0:keyint=180:keyint_min=180:open_gop=0:cabac=1:ref=2\
  :8x8dct=0:vbv-maxrate=128Ki:vbv-bufsize=128Ki:vbv-init=1.0:nal-hrd=cbr" \
  -b:v 300k -maxrate 128Ki -bufsize 128Ki -f h264 frames.h264
```

Put the frames.h264 with the EBOOT in the same folder before reading.  

m-c/d
