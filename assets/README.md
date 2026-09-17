This is the raw assets for the webdev

Use the following command to compress audio file before integrate it with the interface
```
ffmpeg -i input.mp3 -c:a libopus -b:a 12k -ac 1 -application voip output.opus
```