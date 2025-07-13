ffmpeg -re -i test.mpg -f mpegts -vcodec copy -acodec copy udp://127.0.0.1:1234

ffmpeg -i "filepath.mpg" -f data -i "filepath.bin" -filter:v fps=60 -map 0:v:0 -map 1:d:0 -f mpegts udp://IP:PORT


ffmpeg -re -i test.mpg -f data -vcodec copy -acodec copy udp://127.0.0.1:1234
\

ffmpeg -i test.mpg -map data-re -codec copy -f data udp://127.0.0.1:1234