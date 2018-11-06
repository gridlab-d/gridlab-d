REPO=${REPO:-https://codeload.github.com/dchassin/ieee123-aws/zip}
BRANCH=${BRANCH:-master}
echo "
#####################################
# DOCKER BUILD
#   ieee123 <- $REPO/$BRANCH
#####################################
"

# download IEEE 123 model
cd /tmp
curl $REPO/$BRANCH -o ieee123.zip
unzip ieee123.zip
mv ieee123-aws-master/* /var/www/html
rm -rf /tmp/ieee123.zip /tmp/ieee123-aws-master
	
