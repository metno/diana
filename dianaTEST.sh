#!/bin/sh
set -x

input=$0
dianadir="/metno/local/dianaTEST"
test -d $dianadir || dianadir="/usr/local/diana"

export LD_LIBRARY_PATH=/metno/local/lib/mesa:$LD_LIBRARY_PATH
## export LD_LIBRARY_PATH=$dianadir/lib:$LD_LIBRARY_PATH

export DIANADIR=$dianadir

OPSYS=`uname -s`
case $OPSYS in
    IRIX*) metsys="SGI"
	   debugger="dbx"
	   SUBNET=`/usr/etc/ifconfig ec0 | grep "inet" | cut -d. -f3`
	   ;;
    *)	   metsys="Linux"
	   test $dianadir = "/usr/local/diana" && metsys="Linux-Debian"
	   debugger="gdb"
	   SUBNET=`/sbin/ifconfig eth0 | grep "inet addr" | cut -d. -f3`
	   ulimit -c 100000000000
	   ;;
esac

case $SUBNET in
    20)    region="FOU" ;;
    36|40) region="VV" ;;
    48)    region="VNN" ;;
### 50)                        region="MA"  ;;
    16|18|24|26|56|90|104|50) region="VA"  ;;
    *)                         region="VTK" ;;
esac

home=$HOME/grun_op
test -d $home || mkdir $home

home=$home/diana
test -d $home           || mkdir $home
test -d $home/bin       || mkdir $home/bin
test -d $home/bin/work  || mkdir $home/bin/work

cd $home/bin

rm core* 1>/dev/null 2>&1
test -s dbxresult  && mv -f dbxresult dbxresult.old
test -s mail.core  && mv -f mail.core mail.core.old

# remove old lpr print files, left after failure...
find . -name "prt_????-??-??_??:??:??.ps" -mtime +1 -exec rm {} \;

#--------------------------------------------
# to be removed:
ln -sf $dianadir/etc/ANAborders.* .
ln -sf $dianadir/etc/synpltab.dat .
ln -sf $dianadir/etc/metpltab.dat .
#--------------------------------------------
#ln -sf $dianadir/etc/profet.setup-${region} diana.setup
ln -sf $dianadir/etc/diana.setup-${region} diana.setup

tstart=`date`

$dianadir/bin/diana -style cleanlooks 
#-s $dianadir/etc/profet.setup-${region}

tstop=`date`

corefound="no"
for corefile in core*
do
    test -f $corefile || continue
    test -s $corefile || continue
    corefound="yes"
done

if [ $corefound = "yes" ] ; then

echo "USER $USER"     >mail.core
echo "-------------" >>mail.core
uname -a             >>mail.core
echo "-------------" >>mail.core
echo "START $tstart" >>mail.core
echo "STOP  $tstop"  >>mail.core
echo "DianaTEST version (diana.news):" >>mail.core 
cat diana.news       >>mail.core
echo "-------------" >>mail.core
pwd                  >>mail.core
ls -ltr core*        >>mail.core

for corefile in core*
do

test -f $corefile || continue
test -s $corefile || continue

$debugger $dianadir/bin/diana $corefile 1>dbxresult 2>&1 <<ENDDBX
where 
y
y
y
n
q
ENDDBX

echo "===============================================" >>mail.core
cat dbxresult        >>mail.core

rm $corefile

done

echo "===============================================" >>mail.core
hinv                 >>mail.core

adr="audun.christoffersen@met.no helen.korsmo@met.no lisbeth.bergholt@met.no"
Mail -s "DIANA CRASH" $adr < mail.core

fi

exit
