# let's not do all this in /
cd /root

# find out if this is alpine. if it isn't alpine presume debian/ubuntu.
OS_NAME=`cat /etc/os-release | grep  ^'ID=' | cut -d= -f2`

# https://wiki.alpinelinux.org/wiki/Comparison_with_other_distros#Package_management
if [ "$OS_NAME" = "alpine" ]; then
    package_update='apk update'
    package_install='apk add'
    package_delete='apk del'
    required_python='python'
    additional_packages='nano'
else
    package_update='apt-get -y update'
    package_install='apt-get -y install'
    package_delete='apt-get -y remove'
    required_python=''
    additional_packages='software-properties-common
        python-software-properties
        build-essential
        libpq-dev
        libkrb5-dev'
fi

# Update first
$package_update

#
# get commonly useful packages.
#
$package_install \
    git \
    unzip \
    wget \
    curl


# if it's ubuntu then node must be added.
if [ "$OS_SPEC" = "ubuntu" ]; then
    curl -sL https://deb.nodesource.com/setup_$NODE_VERSION.x | bash -
    $package_install nodejs
fi

#
# node-gyp requirements
#
$package_install \
    g++ make git \
    $required_python \
    $additional_packages

# no need for history or any branch other than the one specified
cd /root && git clone --depth 1 -b $AO_BRANCH git://github.com/appoptics/appoptics-bindings-node.git

cd appoptics-bindings-node
echo 'unsafe-perm=true' >> .npmrc
