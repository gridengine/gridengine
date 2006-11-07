#! /bin/csh -f

set mycwd = `pwd`
cd ../..
set arch=`./aimk -no-mk`
cd $mycwd

echo "making configure for $arch"
if ( ! ( -d ${arch} ) ) then
   echo "creating directory ./$arch"
   mkdir $arch
endif

echo "change to directory ./$arch"
cd $arch

echo "calling ../configure --srcdir=.. --with-sge=$arch"
../configure --srcdir=.. --with-sge=$arch

echo "change to directory ./"
cd ..

echo "you should now call 'cd ../.. ; aimk -only-qtcsh'"
