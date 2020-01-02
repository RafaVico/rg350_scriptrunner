#!/bin/sh

OPK_NAME=scriptrunner.opk

echo ${OPK_NAME}

# create default.gcw0.desktop
cat > default.gcw0.desktop <<EOF
[Desktop Entry]
Name=Scriptrunner
Comment=Run scripts and tools
Exec=scriptrunner
Terminal=false
Type=Application
StartupNotify=true
Icon=scriptrunner
Categories=applications;
EOF

# create opk
FLIST="media"
FLIST="${FLIST} scripts"
FLIST="${FLIST} scriptrunner"
FLIST="${FLIST} scriptrunner.png"
FLIST="${FLIST} default.gcw0.desktop"

rm -f ${OPK_NAME}
mksquashfs ${FLIST} ${OPK_NAME} -all-root -no-xattrs -noappend -no-exports

cat default.gcw0.desktop
rm -f default.gcw0.desktop

