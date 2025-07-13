# Decode klvdata using ffmpeg



ffmpeg -re -i test.mpg -map 0 -c copy -f mpegts "udp://@127.0.0.1:1234"



misb\_decode.exe "udp://@127.0.0.1:1234?overrun\_nonfatal=1\&fifo\_size=50000000"





