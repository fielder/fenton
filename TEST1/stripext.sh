#!/usr/bin/env sh

# mv xxx.binary xxx

for x in `/bin/ls *.txt`; do
	/usr/bin/env python3 ../utils/text2bin.py ${x}
	fn=`echo "${x}" | cut -d'.' -f1`
	echo "mv ${fn}.binary ${fn}"
	mv ${fn}.binary ${fn}
done
