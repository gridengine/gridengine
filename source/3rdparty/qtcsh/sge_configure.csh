#! /bin/csh -f
set arch=`../aimk -nomk`

echo "making configure for $arch"
if ( ! ( -d ${arch} ) ) then
   echo "creating directory ./$arch"
   mkdir $arch
endif

echo "change to directory ./$arch"
cd $arch

echo "calling ../configure --srcdir=.. --with-grd=$arch"
../configure --srcdir=.. --with-grd=$arch

echo "change to directory ./"
cd ..

echo "you should now call 'cd .. ; aimk -noqmon qtcsh'"
