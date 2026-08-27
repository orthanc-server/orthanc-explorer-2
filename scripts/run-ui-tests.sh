set -ex

SCRIPT_DIR=$( cd -- "$( dirname -- "$0" )" && pwd )

# example
# To run local tests with Docker:
# ./scripts/run-ui-tests.sh
# ./scripts/run-ui-tests.sh skipBuild=1

skipBuild=0

for argument in "$@"
do
   key=$(echo $argument | cut -f1 -d=)

   key_length=${#key}
   value="${argument:$key_length+1}"

   export "$key"="$value"
done


if [[ $skipBuild == "0" ]]; then
    pushd $SCRIPT_DIR/..
    docker build -t orthancteam/orthanc-pre-release:latest-oe2-for-tests .
    popd
fi

pushd $SCRIPT_DIR/../tests/

if [ ! -d ".venv" ]; then
    python -m venv .venv
fi

source .venv/bin/activate
pip install -r requirements.txt

playwright install

pytest . -v -s --orthanc-mode=orthanc-docker --no-header
# pytest test_auth_service_no_emails.py -v -s --orthanc-mode=orthanc-docker --no-header


echo $(pwd)

popd