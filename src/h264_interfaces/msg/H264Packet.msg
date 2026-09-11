std_msgs/Header header

# Frame dimensions used to initialize an MP4 video stream.
uint32 width
uint32 height

# Presentation timestamp in nanoseconds. It is monotonic within one stream.
uint64 pts_ns
uint64 duration_ns
bool keyframe

# H.264 codec extradata (SPS/PPS in the format expected by FFmpeg's MP4 muxer).
# It is repeated so a recorder that starts late can initialize its muxer.
uint8[] codec_config

# One encoded H.264 access unit.
uint8[] data
