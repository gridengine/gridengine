#/bin/csh
set argument1=$1

if ( ! $?SGE_ROOT ) then
   echo "please set SGE_ROOT"
   exit 1
endif

if ( ! $?SGE_QMASTER_PORT ) then
   echo "please set SGE_QMASTER_PORT"
   exit 1
endif

if ( ! $?USER  ) then
   echo "please set SGE_QMASTER_PORT"
   exit 1
endif


if ( $argument1 == "client" ) then
   echo "setting client environment"
   setenv SSL_CA_CERT_FILE "$SGE_ROOT/default/common/sgeCA/cacert.pem"
   setenv SSL_CERT_FILE "/var/sgeCA/port$SGE_QMASTER_PORT/default/userkeys/$USER/cert.pem"
   setenv SSL_KEY_FILE "/var/sgeCA/port$SGE_QMASTER_PORT/default/userkeys/$USER/key.pem"
   setenv SSL_RAND_FILE "/var/sgeCA/port$SGE_QMASTER_PORT/default/userkeys/$USER/rand.seed"
   exit 0
endif

if ( $argument1 == "service" ) then
   echo "setting service environment"
   setenv SSL_CA_CERT_FILE "$SGE_ROOT/default/common/sgeCA/cacert.pem"
   setenv SSL_CERT_FILE "$SGE_ROOT/default/common/sgeCA/certs/cert.pem"
   setenv SSL_KEY_FILE "/var/sgeCA/port$SGE_QMASTER_PORT/default/private/key.pem"
   setenv SSL_RAND_FILE "/var/sgeCA/port$SGE_QMASTER_PORT/default/private/rand.seed"
   exit 0
endif

echo "usage: certificate.sh client | service"

# $SGE_ROOT/utilbin/$ARCH/openssl s_client -port $SGE_QMASTER_PORT -cert $SSL_CERT_FILE -key $SSL_KEY_FILE
# $SGE_ROOT/utilbin/$ARCH/openssl s_server -port $CL_PORT -cert $SSL_CERT_FILE -key $SSL_KEY_FILE -state -nbio -verify true -CAfile $SSL_CA_CERT_FILE

