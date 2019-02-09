# specify all arguments as env vars on command line so they look
# like keyword paramaters.
OSes=${OSes:-"debian"} #debian ubuntu alpine
EXECUTE=${ACTION:-"build-bindings"} # build-bindings, release-bindings, simulate, build-image
VERSIONS=${VERSIONS:-"6"} # any are ok but stick with 6, 8, 10
current_branch=$(git branch | grep \* | cut -d ' ' -f2)
BRANCH=${BRANCH:-"$current_branch"}
PULL=${PULL-yes}

echo "build appoptics-bindings.node using node versions $VERSIONS and OSes $OSes"

# provide a unique index to each image. it can be used to avoid
# database/table name conflicts when multiple db tests are executing
# simultaneously.

[ ! "$OSes" = "debian" ] && echo "this is untested except for debian" && exit 1

for os in $OSes
do

    # now setup the node versions requested
    for version in $VERSIONS
    do
        # make the image name based on the os and version
        if [ "$os" = "debian" ]; then
            # this uses node's official version as debian is relatively current
            image=node:$version-jessie
        elif [ "$os" = "ubuntu" ]; then
            # this uses nodesource's installation as node doesn't have an ubuntu image
            image=ubuntu:16.04
        elif [ "$os" = "alpine" ]; then
            # this uses mhart's because node's alpine is an old version
            image=mhart/alpine-node:$version
        else
            echo "$os is not a known option for operating system"
            exit 1
        fi
        tag=bindings-$version-$os-build
        if [ $EXECUTE = "build-bindings" -o $EXECUTE = "release-bindings" ]; then
            # just build the bindings
            echo "starting build in: $tag"
            uuid=$(docker run -d \
                -v $PWD/build/$os:/root/appoptics-bindings-node/output \
                -e PULL \
                $tag)
            docker wait $uuid
            if [ $? -eq 0 -a $EXECUTE = "release-bindings" ]; then
                echo "copying $PWD/build/$os/appoptics-bindings.node to $PWD/build/Release/"
                cp $PWD/build/$os/appoptics-bindings.node $PWD/build/Release/
            fi
        elif [ $EXECUTE = "simulate" ]; then
            # just say what would be done
            echo "SIMULATE: build appoptics-bindings.node using $tag on branch $BRANCH"
        elif [ $EXECUTE = "build-image" ]; then
          # build the image. this generally only needs to be done once; it's mostly
          # appoptics-bindings.node that will be built, i.e., "build-bindings".
          docker build . -t $tag \
              --build-arg AO_BRANCH=$BRANCH \
              --build-arg NODE_VERSION=$version \
              --build-arg OS_SPEC=$os \
              --build-arg IMAGE=$image \
              --build-arg TAG=$tag
        else
            echo "Invalid ACTION '$EXECUTE'"
            echo "Valid options are build-image, simulate, build-bindings, release-bindings"
            echo ""
            echo "Normally you will first execute ACTION=build-image ./release-tool.sh"
            echo "then ./release-tool.sh (the default ACTION is build-bindings)"
            echo ""
            echo "the default is for git to issue a pull command before building"
            echo "if you don't want this use PULL=\"\" ./release-tool.sh"
            exit 1
        fi
    done
done

exit 0
