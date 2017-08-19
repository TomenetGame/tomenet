#plowdown http://www.mediafire.com/?3j87kp3fgzpqrqn

cd dl_posix
./download.sh http://www.mediafire.com/?3j87kp3fgzpqrqn
RETVAL=$?
echo $RETVAL > ../result.tmp
mv *.7z ..
