#!/bin/bash

echo
echo "ClumpyM3U - The Playlist Clumpifier"


H=`pwd`


update () {
 apt update
 apt install gcc build-essential
}

compile () {
 gcc clumpy.c -o clumpy -Os -fomit-frame-pointer
 ls -l clumpy
}

askDir () {
 F=`zenity --file-selection --directory`
 cd "$F"
}

run () {
 ls *.mp3 *.ogg *.wav *.webm *.m4a *.flac \
	2>/dev/null | sort -R > playlist.m3u
 $H/clumpy -i playlist.m3u -o out.m3u
}

play () {
 xdg-open clumpy.m3u
}


while true
do
 echo
 echo " Menu"
 echo "i Install or update GCC compiler via apt"
 echo "c Compile via GCC"
 echo "d Select directory"
 echo "r Run (Make clumpy playlist)"
 echo "h Show help file"
 echo "w Open web page on github"
 echo "q Quit"

 read -n 1 -p "?" i
 echo

 case $i in
  i) update ;;
  c) compile ;;
  d) askDir ;;
  r) run ;;
  h) xdg-open doc.htm ;;
  w) xdg-open "https://github.com/RonAmerica/ClumpyM3U" ;;
  q) break ;;
  *) echo "Bad user!" ;;
 esac

done

echo
echo "Done."
exit

