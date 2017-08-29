#plowdown http://www.mediafire.com/?issv5sdv7kv3odq

cd dl_posix
./download.sh http://www.mediafire.com/?issv5sdv7kv3odq
RETVAL=$?
echo $RETVAL > ../result.tmp
mv *.7z ..
