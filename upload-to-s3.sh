#!/bin/bash

#Based on https://gist.github.com/chrismdp/6c6b6c825b07f680e710
function uploadToS3 {
  file=$1
  aws_path=$2
  aws_id=$3
  aws_token=$4
  aws_bucket=$5
  date=$(date +"%a, %d %b %Y %T %z")
  acl="x-amz-acl:public-read"
  content_type='application/x-compressed-tar'
  string="PUT\n\n$content_type\n$date\n$acl\n/$aws_bucket$aws_path$file"
  signature=$(echo -en "${string}" | openssl sha1 -hmac "${aws_token}" -binary | base64)
  curl -X PUT -T "$file" \
    -H "Host: $aws_bucket.s3.amazonaws.com" \
    -H "Date: $date" \
    -H "Content-Type: $content_type" \
    -H "$acl" \
    -H "Authorization: AWS ${aws_id}:$signature" \
    "https://$aws_bucket.s3.amazonaws.com$aws_path$file"
  echo "Uploaded to https://$aws_bucket.s3.amazonaws.com$aws_path$file"
}

if [ -z "$VERSION" ];
then
  echo "VERSION not set, going with git tags as version";
  VERSION=`git describe --tags`
fi

if [ -z "$TARGET" ];
then
  echo "TARGET not set, going with system name";
  TARGET=`uname -n`
fi

if [ -z "$AMAZON_API_TOKEN" ];
then
  echo "Amazon API Token not provided, skipping upload";
else
  if [ ! -f iconographer-${VERSION}-${TARGET}.tgz ];
  then
    echo "Iconographer tarball for this version was not found!"
  else
    uploadToS3 "iconographer-${VERSION}-${TARGET}.tgz" "/bundles/" $AMAZON_API_ID $AMAZON_API_TOKEN $AMAZON_API_BUCKET
    echo "Success!"
  fi
fi
echo "End."
