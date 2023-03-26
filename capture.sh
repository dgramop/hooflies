for i in {1..50} 
do 
	nc -l 0.0.0.0 8001 > /tmp/jpegs/$i.jpeg;
done

ffmpeg -framerate 12 -i /tmp/jpegs/%d.jpeg output.mp4
