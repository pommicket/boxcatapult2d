#!/bin/sh
make clean
make release -j8 || exit 1
tar --transform 's,^,boxcatapult2d/,' -czvf boxcatapult2d-source.tar.gz $(git ls-files)
tar --transform 's,^,boxcatapult2d/,' -czvf boxcatapult2d-linux.tar.gz \
	$(git ls-files assets) boxcatapult2d LICENSE README.md

