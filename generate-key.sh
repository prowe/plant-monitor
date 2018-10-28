#!/bin/bash

set -e

rm -f *.key
rm -f *.pem

aws iot create-keys-and-certificate \
    --certificate-pem-outfile thing.cert.pem \
    --public-key-outfile thing.public.key \
    --private-key-outfile thing.private.key \
    --set-as-active

openssl x509 -in thing.cert.pem -out data/thing.der -outform DER
openssl rsa -in thing.private.key -out data/thing.private.der -outform DER