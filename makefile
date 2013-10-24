#
#
#

#
FLAGS = -O3 -Wno-unused-result

OPENCV = -lopencv_highgui -lopencv_core -lopencv_imgproc -I/usr/include/opencv

#
#
#

output:
	$(CC) main.c rotodet.c -lm $(OPENCV) $(FLAGS) -o exe